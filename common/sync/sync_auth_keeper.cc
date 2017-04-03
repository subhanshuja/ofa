// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sync_auth_keeper.h"

#include <algorithm>
#include <limits>

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/histogram.h"
#include "components/prefs/pref_service.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/driver/sync_client.h"

#include "common/account/account_service.h"
#include "common/sync/pref_names.h"
#include "common/sync/sync_account.h"
#include "common/sync/sync_auth_keeper_event_recorder.h"
#include "common/sync/sync_auth_keeper_observer.h"
#include "common/sync/sync_config.h"
#include "common/sync/sync_login_data.h"
#include "common/sync/sync_password_recoverer.h"

namespace opera {
namespace sync {
SyncAuthKeeper::SyncAuthKeeper(syncer::SyncService* sync_service,
                               PrefService* pref_service)
    : account_(nullptr),
      auth_data_recovery_backoff_policy_(net::BackoffEntry::Policy()),
      auth_data_recovery_retry_reason_(ADRRR_UNKNOWN),
      last_session_description_(pref_service),
      next_auth_data_recovery_time_(base::Time()),
      password_recoverer_(nullptr),
      periodic_recovery_count_(0),
      pref_service_(pref_service),
      recovery_limit_period_start_time_(base::Time()),
      sync_service_(sync_service),
      token_lost_reason_(TLR_UNKNOWN),
      token_state_(TOKEN_STATE_UNKNOWN),
      initialized_(false),
      weak_factory_(this) {
  DCHECK(opera::SyncConfig::ShouldUseAuthTokenRecovery());
  DCHECK(pref_service_);
  DCHECK(sync_service_);
  DCHECK(sync_service_->GetSyncClient());

  VLOG(1) << "Auth keeper created.";
  ResetRecoveryLimit();

  InitAuthDataRecoveryBackoffPolicy();
  auth_data_recovery_backoff_.reset(
      new net::BackoffEntry(&auth_data_recovery_backoff_policy_));
}

SyncAuthKeeper::~SyncAuthKeeper() {}

void SyncAuthKeeper::Initialize() {
  DCHECK(opera::SyncConfig::ShouldUseAuthTokenRecovery());
  VLOG(1) << "Auth keeper initializing.";

  account_ = sync_service_->GetSyncClient()->GetSyncAccount();
  DCHECK(account_);
  DCHECK(account_->service());
  account_->service()->AddObserver(this);
  LoadPrefs();

  if (token_state() == TOKEN_STATE_UNKNOWN) {
    if (account_->HasAuthData()) {
      DCHECK(account_->HasValidAuthData());
      token_state_ = TOKEN_STATE_OK;
    } else {
      token_state_ = TOKEN_STATE_LOGGED_OUT;
    }
  }

  event_recorder_.reset(new SyncAuthKeeperEventRecorder());
  AddObserver(event_recorder_.get());

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(&SyncAuthKeeper::NotifyInitializationComplete,
                            weak_factory_.GetWeakPtr()));

  initialized_ = true;
}

void SyncAuthKeeper::Shutdown() {
  VLOG(1) << "Auth keeper shutting down.";

  DCHECK(event_recorder_.get());
  RemoveObserver(event_recorder_.get());
  event_recorder_.reset();

  SavePrefs();

  password_recoverer_ = nullptr;
  account_->service()->RemoveObserver(this);
  account_ = nullptr;
}

void SyncAuthKeeper::OnLoggedIn(AccountService* service,
                                opera_sync::OperaAuthProblem problem) {
  DCHECK(opera::SyncConfig::ShouldUseAuthTokenRecovery());
  DCHECK(account_);
  DCHECK(account_->service());
  DCHECK_EQ(service, account_->service().get());
  DCHECK(!password_recoverer_);

  VLOG(1) << "On logged in for login '" << current_account_login()
          << "' with problem '" << problem.ToString() << "'";

  token_refresh_description_.clear();
  token_refresh_description_.set_success_time(base::Time::Now());
  token_refresh_description_.set_request_problem(problem);

  password_recoverer_ = nullptr;
  recovery_retry_timer_.Stop();
  auth_data_recovery_retry_reason_ = ADRRR_UNKNOWN;
  token_lost_reason_ = TLR_UNKNOWN;
  ChangeTokenState(TOKEN_STATE_OK);

  MaybeNotifyStatusChanged();
  FOR_EACH_OBSERVER(SyncAuthKeeperObserver, observers_,
                    OnLoggedIn(service, problem));
}

void SyncAuthKeeper::OnLoginError(AccountService* service,
                                  account_util::AuthDataUpdaterError error_code,
                                  account_util::AuthOperaComError auth_code,
                                  const std::string& message,
                                  opera_sync::OperaAuthProblem problem) {
  DCHECK(opera::SyncConfig::ShouldUseAuthTokenRecovery());
  DCHECK_EQ(service, account_->service().get());

  VLOG(1) << "Auth keeper login error error_code="
          << account_util::AuthDataUpdaterErrorToString(error_code)
          << "; auth_code="
          << account_util::AuthOperaComErrorToString(auth_code)
          << "; message = '" << message
          << "'; problem = '" + problem.ToString() + "'";

  token_refresh_description_.clear();
  token_refresh_description_.set_error_time(base::Time::Now());
  token_refresh_description_.set_error_type(error_code);
  token_refresh_description_.set_auth_error_type(auth_code);
  token_refresh_description_.set_request_error_message(message);
  token_refresh_description_.set_request_problem(problem);

  MaybeNotifyStatusChanged();

  switch (error_code) {
    case account_util::ADUE_AUTH_ERROR:
      OnAuthError(auth_code);
      break;
    case account_util::ADUE_INTERNAL_ERROR:
      // This appears to be an unlikely condition when user managed to log out
      // before the auth data arrived for SyncAccount. We don't want to recover
      // from this state with an auto-login.
      DCHECK_EQ(token_state(), TOKEN_STATE_LOGGED_OUT);
      VLOG(1) << "Ignoring internal error, user should be logged out by now";
      break;
    case account_util::ADUE_NETWORK_ERROR:
    case account_util::ADUE_HTTP_ERROR:
    case account_util::ADUE_PARSE_ERROR:
      // These are interpreted as temporary auth errors that are to be gone
      // after a retry, compare ProfileSyncService::OnLoginError().
      break;
    default:
      NOTREACHED();
      break;
  }
  MaybeNotifyStatusChanged();
  FOR_EACH_OBSERVER(
      SyncAuthKeeperObserver, observers_,
      OnLoginError(service, error_code, auth_code, message, problem));
}

void SyncAuthKeeper::OnLoggedOut(AccountService* service,
                                 account_util::LogoutReason logout_reason) {
  DCHECK(opera::SyncConfig::ShouldUseAuthTokenRecovery());
  DCHECK_EQ(service, account_->service().get());
  VLOG(1) << "On logged out for login '" << current_account_login()
          << "' with reason "
          << account_util::LogoutReasonToString(logout_reason);

  token_refresh_description_.clear();

  // Effectively cancel any pending password recovery, we'll ignore callbacks
  // from any recoverer that is not our current password_recoverer_.
  password_recoverer_ = nullptr;
  recovery_retry_timer_.Stop();
  auth_data_recovery_retry_reason_ = ADRRR_UNKNOWN;
  ChangeTokenState(TOKEN_STATE_LOGGED_OUT);

  MaybeNotifyStatusChanged();
  FOR_EACH_OBSERVER(SyncAuthKeeperObserver, observers_,
                    OnLoggedOut(service, logout_reason));
}

void SyncAuthKeeper::OnAuthDataExpired(AccountService* service,
                                       opera_sync::OperaAuthProblem problem) {
  DCHECK(opera::SyncConfig::ShouldUseAuthTokenRecovery());
  DCHECK_EQ(service, account_->service().get());
  VLOG(1) << "On auth data expired for login '" << current_account_login()
          << "' with problem '" << problem.ToString() << "'";

  token_refresh_description_.clear();
  token_refresh_description_.set_request_problem(problem);
  token_refresh_description_.set_expired_time(base::Time::Now());

  ChangeTokenState(TOKEN_STATE_REFRESHING);
  MaybeNotifyStatusChanged();
  FOR_EACH_OBSERVER(SyncAuthKeeperObserver, observers_,
                    OnAuthDataExpired(service, problem));
}

void SyncAuthKeeper::OnLoginRequested(AccountService* service,
                                      opera_sync::OperaAuthProblem problem) {
  DCHECK(opera::SyncConfig::ShouldUseAuthTokenRecovery());
  DCHECK_EQ(service, account_->service().get());
  VLOG(1) << "On login requested for login '" << current_account_login()
          << "' with problem " << problem.ToString();

  token_refresh_description_.clear();
  token_refresh_description_.set_request_problem(problem);
  token_refresh_description_.set_request_time(base::Time::Now());
  token_refresh_description_.set_request_problem(problem);

  MaybeNotifyStatusChanged();
  FOR_EACH_OBSERVER(SyncAuthKeeperObserver, observers_,
                    OnLoginRequested(service, problem));
}

void SyncAuthKeeper::OnLogoutRequested(
    AccountService* service,
    account_util::LogoutReason logout_reason) {
  DCHECK(opera::SyncConfig::ShouldUseAuthTokenRecovery());
  DCHECK_EQ(service, account_->service().get());
  VLOG(1) << "On logout requested for login '" << current_account_login()
          << "' with reason "
          << account_util::LogoutReasonToString(logout_reason);

  token_refresh_description_.clear();

  UpdateLastSessionDescription(get_current_session_duration(),
                               account_->login_data().session_id,
                               logout_reason);

  UMA_HISTOGRAM_ENUMERATION("Opera.DSK.Sync.Logout.Reason", logout_reason,
                            account_util::LR_LAST_ENUM_VALUE);

  if (get_current_session_duration() != base::TimeDelta()) {
    UMA_HISTOGRAM_BOOLEAN("Opera.DSK.Sync.SessionDurationAvailable", true);
    int current_session_duration_in_minutes =
        get_current_session_duration().InMinutes();
    // TODO(mzajaczkowski): 10 buckets?
    UMA_HISTOGRAM_CUSTOM_COUNTS("Opera.DSK.Sync.SessionDuration",
                                current_session_duration_in_minutes, 0,
                                std::numeric_limits<int>::max() - 1, 10);
  } else {
    UMA_HISTOGRAM_BOOLEAN("Opera.DSK.Sync.SessionDurationAvailable", false);
  }

  MaybeNotifyStatusChanged();
  FOR_EACH_OBSERVER(SyncAuthKeeperObserver, observers_,
                    OnLogoutRequested(service, logout_reason));
}

void SyncAuthKeeper::OnLoginRequestDeferred(
    AccountService* service,
    opera_sync::OperaAuthProblem problem,
    base::Time scheduled_time) {
  DCHECK(opera::SyncConfig::ShouldUseAuthTokenRecovery());
  DCHECK_EQ(service, account_->service().get());
  VLOG(1) << "On login requested deferred for login '"
          << current_account_login() << "' with problem " << problem.ToString();

  token_refresh_description_.clear();
  token_refresh_description_.set_request_problem(problem);
  token_refresh_description_.set_scheduled_request_time(scheduled_time);

  MaybeNotifyStatusChanged();
  FOR_EACH_OBSERVER(SyncAuthKeeperObserver, observers_,
                    OnLoginRequestDeferred(service, problem, scheduled_time));
}

void SyncAuthKeeper::OnSyncPasswordRecoveryFinished(
    scoped_refptr<SyncPasswordRecoverer> recoverer,
    SyncPasswordRecoverer::Result result,
    const std::string& username,
    const std::string& password,
    const std::string& token,
    const std::string& token_secret) {
#if defined(OPERA_DESKTOP)
  DCHECK(opera::SyncConfig::ShouldUseAuthTokenRecovery());
  DCHECK(recoverer);
  VLOG(1) << "Recovery finished for '" + username + "' with result "
          << SyncPasswordRecovererResultToString(result);
  if (recoverer != password_recoverer_) {
    VLOG(1) << "Ignoring cancelled password recovery.";
    return;
  }

  // Drop the reference, the recoverer will delete itself.
  password_recoverer_ = nullptr;

  if (!account_->IsLoggedIn()) {
    // Is this even possible?
    VLOG(1) << "User did log out in the meantime, ignoring.";
    return;
  }

  DCHECK_EQ(username, recoverer->user_name());
  DCHECK_EQ(username, current_account_login());

  switch (result) {
    case SyncPasswordRecoverer::SPRR_RECOVERED:
      DCHECK(!password.empty());
      VLOG(1) << "Recovery successul for login '" + current_account_login() +
                     "' with new token '" + token + "'.";
      // We should never get empty auth data here, but better safe than sorry.
      if (token.empty() || token_secret.empty()) {
        NOTREACHED() << "Got no auth data after password recovery.";
        // Give up, this is strange enough already.
        MarkTokenLost(TLR_NO_AUTH_DATA_AFTER_RECOVERY);
        auth_data_recovery_backoff_->InformOfRequest(false);
      } else {
        // Use the auth data.
        SyncLoginData recovered_data = account_->login_data();
        DCHECK_EQ(recovered_data.display_name(), current_account_login());
        recovered_data.auth_data.token = token;
        recovered_data.auth_data.token_secret = token_secret;
        VLOG(1) << "Performing login with new auth data.";
        account_->LogInWithAuthData(recovered_data);
        // Token state will be updated due to account calling us back.
        auth_data_recovery_backoff_->InformOfRequest(true);
        ChangeTokenState(TOKEN_STATE_OK);

        UMA_HISTOGRAM_BOOLEAN("Opera.DSK.Sync.Recovery.Success", true);
      }
      break;
    case SyncPasswordRecoverer::SPRR_HTTP_ERROR:
      VLOG(1) << "Auth data recovery HTTP failure, scheduling retry.";
      ScheduleAuthDataRecoveryRetry(ADRRR_HTTP_ERROR);
      break;
    case SyncPasswordRecoverer::SPRR_NETWORK_ERROR:
      VLOG(1) << "Auth data recovery NETWORK failure, scheduling retry.";
      ScheduleAuthDataRecoveryRetry(ADRRR_NETWORK_ERROR);
      break;
    case SyncPasswordRecoverer::SPRR_PARSE_ERROR:
      VLOG(1) << "Auth data recovery PARSE failure, scheduling retry.";
      ScheduleAuthDataRecoveryRetry(ADRRR_PARSE_ERROR);
      break;
    case SyncPasswordRecoverer::SPRR_CREDENTIALS_NOT_FOUND:
      MarkTokenLost(TLR_CREDENTIALS_NOT_FOUND);
      break;
    case SyncPasswordRecoverer::SPRR_CREDENTIALS_INCORRECT:
      MarkTokenLost(TLR_CREDENTIALS_INCORRECT);
      break;
    case SyncPasswordRecoverer::SPRR_INTERNAL_ERROR:
      MarkTokenLost(TLR_INTERNAL_ERROR);
      break;
    case SyncPasswordRecoverer::SPRR_PASSWORD_MANAGER_TIMEOUT:
      MarkTokenLost(TLR_PASSWORD_MANAGER_TIMEOUT);
      break;
    case SyncPasswordRecoverer::SPRR_UNKNOWN:
    default:
      MarkTokenLost(TLR_UNKNOWN_PASSWORD_RECOVERY_RESULT);
      NOTREACHED() << result;
      break;
  }
  MaybeNotifyStatusChanged();
#endif  // OPERA_DESKTOP
}

void SyncAuthKeeper::AddObserver(SyncAuthKeeperObserver* observer) {
  DCHECK(opera::SyncConfig::ShouldUseAuthTokenRecovery());
  observers_.AddObserver(observer);
}

void SyncAuthKeeper::RemoveObserver(SyncAuthKeeperObserver* observer) {
  DCHECK(opera::SyncConfig::ShouldUseAuthTokenRecovery());
  observers_.RemoveObserver(observer);
}

void SyncAuthKeeper::MaybeNotifyStatusChanged() {
  if (!initialized_) {
    VLOG(1)
        << "No status change broadcasts until service is fully constructed.";
    return;
  }

  const SyncAuthKeeperStatus cur_status = status();
  if (prev_status_.IsSameAs(cur_status)) {
    return;
  }
  prev_status_ = cur_status;
  FOR_EACH_OBSERVER(SyncAuthKeeperObserver, observers_,
                    OnSyncAuthKeeperStatusChanged(this));
}

SyncAuthKeeperStatus SyncAuthKeeper::status() const {
  DCHECK(opera::SyncConfig::ShouldUseAuthTokenRecovery());
  SyncAuthKeeperStatus out;
  out.set_account__auth_server_url(SyncConfig::AuthServerURL().spec());
  out.set_account__current_session_duration(get_current_session_duration());
  out.set_account__has_auth_data(account_->HasAuthData());
  out.set_account__has_valid_auth_data(account_->HasValidAuthData());
  out.set_account__is_logged_in(account_->IsLoggedIn());
  out.set_account__session_id(account_->IsLoggedIn()
                                  ? account_->login_data().session_id
                                  : kNotAvailableString);
  out.set_account__token(account_->login_data().auth_data.token);
  out.set_account__used_username_for_login(
      account_->login_data().used_username_to_login);
  out.set_account__user_email(account_->login_data().user_email);
  out.set_account__user_id(account_->login_data().user_id);
  out.set_account__user_name(account_->login_data().user_name);

  out.set_recovery__next_attempt_time(next_auth_data_recovery_time_);
  out.set_recovery__token_state(token_state_string());
  out.set_recovery__token_lost_reason(
      TokenLostReasonToString(token_lost_reason_));
  out.set_recovery__retry_reason(
      AuthDataRecoveryRetryReasonToString(auth_data_recovery_retry_reason_));
  out.set_recovery__retry_timer_running(recovery_retry_timer_.IsRunning());
  out.set_recovery__periodic_recovery_count(periodic_recovery_count_);

  out.set_refresh__expired_time(token_refresh_description_.expired_time());
  out.set_refresh__request_time(token_refresh_description_.request_time());
  out.set_refresh__expired_reason(
      token_refresh_description_.request_problem_reason_string());
  out.set_refresh__expired_source(
      token_refresh_description_.request_problem_source_string());
  out.set_refresh__expired_token(
      token_refresh_description_.request_problem_token_string());
  out.set_refresh__scheduled_request_time(
      token_refresh_description_.scheduled_request_time());
  out.set_refresh__request_success_time(
      token_refresh_description_.success_time());
  out.set_refresh__request_error_time(token_refresh_description_.error_time());
  out.set_refresh__request_error_type(
      token_refresh_description_.error_type_string());
  out.set_refresh__request_auth_error_type(
      token_refresh_description_.auth_error_type_string());
  out.set_refresh__request_error_message(
      token_refresh_description_.request_error_message_string());

  out.set_last_session__duration(last_session_description_.duration());
  out.set_last_session__session_id(last_session_description_.id_string());
  out.set_last_session__logout_reason(
      last_session_description_.logout_reason_string());

  out.set_ui__status(sync_service_->opera_sync_status().ToString());

  out.set_timestamp(base::Time::Now().ToDoubleT());

  return out;
}

SyncAuthKeeper::TokenState SyncAuthKeeper::token_state() const {
  DCHECK(opera::SyncConfig::ShouldUseAuthTokenRecovery());
  return token_state_;
}

std::string SyncAuthKeeper::token_state_string() const {
  DCHECK(opera::SyncConfig::ShouldUseAuthTokenRecovery());
  return TokenStateToString(token_state_);
}

bool SyncAuthKeeper::is_token_lost() const {
  VLOG(5) << "Current token state is " << token_state_string();
  return token_state_ == TokenState::TOKEN_STATE_LOST;
}

SyncAuthKeeperEventRecorder* SyncAuthKeeper::event_recorder() const {
  DCHECK(event_recorder_.get());
  return event_recorder_.get();
}

void SyncAuthKeeper::LoadPrefs() {
  int saved_token_lost_reason =
      pref_service_->GetInteger(kOperaSyncAuthTokenLostReasonPref);
  if (saved_token_lost_reason >= 0 &&
      saved_token_lost_reason < TLR_LAST_ENUM_VALUE)
    token_lost_reason_ = static_cast<TokenLostReason>(saved_token_lost_reason);

  bool token_was_lost = pref_service_->GetBoolean(kOperaSyncAuthTokenLostPref);
  if (token_was_lost) {
    DCHECK_NE(token_lost_reason_, TLR_UNKNOWN) << token_lost_reason_;
    token_state_ = TOKEN_STATE_LOST;
  } else {
    DCHECK_EQ(token_lost_reason_, TLR_UNKNOWN) << token_lost_reason_;
    // We don't know if it's logged out or OK yet.
  }
}

void SyncAuthKeeper::SavePrefs() {
  pref_service_->SetBoolean(kOperaSyncAuthTokenLostPref, is_token_lost());
  int token_lost_reason_value = static_cast<int>(token_lost_reason_);
  pref_service_->SetInteger(kOperaSyncAuthTokenLostReasonPref,
                            token_lost_reason_value);
}

const std::string SyncAuthKeeper::current_account_login() const {
  DCHECK(opera::SyncConfig::ShouldUseAuthTokenRecovery());
  DCHECK(account_);
  return account_->login_data().display_name();
}

void SyncAuthKeeper::StartAuthDataRecovery() {
  DCHECK(opera::SyncConfig::ShouldUseAuthTokenRecovery());
  // We don't want to do this after a LogOut() was performed.
  DCHECK(account_->IsLoggedIn());
  const std::string current_login = current_account_login();
  DCHECK(!current_login.empty());
  VLOG(1) << "Starting recovery for '" << current_login << "'.";

  if (AuthDataRecoveryLimitReached()) {
    MarkTokenLost(TLR_RECOVERY_LIMIT_REACHED);
    return;
  }

  periodic_recovery_count_++;

  next_auth_data_recovery_time_ = base::Time();

#if defined(OPERA_DESKTOP)
  // Cancel any recovery if in progress.
  if (password_recoverer_) {
    VLOG(3) << "Cancelling recovery for user '"
            << password_recoverer_->user_name() << "'";
    password_recoverer_ = nullptr;
  }

  password_recoverer_ =
      sync_service_->GetSyncClient()->CreateOperaSyncPasswordRecoverer();
  SyncPasswordRecoverer::RecoveryFinishedCallback callback =
      base::Bind(&SyncAuthKeeper::OnSyncPasswordRecoveryFinished,
                 weak_factory_.GetWeakPtr());
  bool ok = password_recoverer_->TryRecover(current_login, callback);
  if (ok) {
    ChangeTokenState(TOKEN_STATE_RECOVERING);
  } else {
    // No can do, no password store.
    VLOG(1) << "Password recoverer could not start recovery.";
    DCHECK_EQ(password_recoverer_->result(),
              SyncPasswordRecoverer::SPRR_INTERNAL_ERROR);
    MarkTokenLost(TLR_NO_PASSWORD_STORE);
  }
#endif  // OPERA_DESKTOP
  MaybeNotifyStatusChanged();
}

void SyncAuthKeeper::ChangeTokenState(SyncAuthKeeper::TokenState token_state) {
  DCHECK(opera::SyncConfig::ShouldUseAuthTokenRecovery());
  if (token_state_ == token_state) {
    VLOG(5) << "Ignoring token state change to "
            << TokenStateToString(token_state)
            << " since this is already is current state.";
    return;
  }

  VLOG(2) << "Token state changed from " << token_state_string() << " to "
          << TokenStateToString(token_state);

  if (token_state_ == TOKEN_STATE_LOST && token_state == TOKEN_STATE_OK) {
    periodic_recovery_count_ = 0;
  }

  token_state_ = token_state;
  FOR_EACH_OBSERVER(SyncAuthKeeperObserver, observers_,
                    OnSyncAuthKeeperTokenStateChanged(this, token_state_));
  MaybeNotifyStatusChanged();
}

void SyncAuthKeeper::ChangeTokenLostReason(TokenLostReason reason) {
  DCHECK_EQ(token_lost_reason_, TLR_UNKNOWN);
  token_lost_reason_ = reason;
  MaybeNotifyStatusChanged();
}

void SyncAuthKeeper::OnAuthError(account_util::AuthOperaComError auth_code) {
  DCHECK(opera::SyncConfig::ShouldUseAuthTokenRecovery());
  switch (auth_code) {
    case account_util::AOCE_420_NOT_AUTHORIZED_REQUEST:
    case account_util::AOCE_421_BAD_REQUEST:
    case account_util::AOCE_422_OPERA_USER_NOT_FOUND:
    case account_util::AOCE_426_COULD_NOT_GENERATE_OPERA_TOKEN:
    case account_util::AOCE_428_OPERA_ACCESS_TOKEN_NOT_EXPIRED:
      UMA_HISTOGRAM_ENUMERATION("Opera.DSK.Sync.Recovery.StartReason",
                                auth_code, account_util::AOCE_LAST_ENUM_VALUE);
      StartAuthDataRecovery();
      break;
    case account_util::AOCE_424_OPERA_TOKEN_NOT_FOUND:
      // Token was explicitly not found on the server.
      MarkTokenLost(TLR_TOKEN_NOT_FOUND_ON_SERVER);
      break;
    case account_util::AOCE_425_INVALID_OPERA_TOKEN:
      // Token was explicitly marked as invalid by the remote service.
      // Don't try to recover.
      MarkTokenLost(TLR_TOKEN_INVALID);
      break;
    case account_util::AOCE_AUTH_ERROR_NOT_RECOGNIZED:
      VLOG(1) << "Unknown auth code from remote service received.";
      MarkTokenLost(TLR_AUTH_ERROR_NOT_RECOGNIZED);
      break;
    case account_util::AOCE_AUTH_ERROR_SIMPLE_REQUEST:
      // We're not using command line login with ProfileSyncService
      // anymore.
      NOTREACHED() << "Unknown auth code from remote service received.";
      MarkTokenLost(TLR_GOT_SIMPLE_AUTH_ERROR);
      break;
    case account_util::AOCE_NO_ERROR:
      // Should not happen as this method should be only called for auth errors.
      NOTREACHED() << "Got no error as auth error.";
      MarkTokenLost(TLR_GOT_NO_AUTH_ERROR);
      break;
    default:
      // Stay safe.
      NOTREACHED() << "Unrecognized auth error.";
      MarkTokenLost(TLR_AUTH_ERROR_NOT_RECOGNIZED);
      break;
  }
}

void SyncAuthKeeper::InitAuthDataRecoveryBackoffPolicy() {
  auth_data_recovery_backoff_policy_.num_errors_to_ignore = 0;
  auth_data_recovery_backoff_policy_.initial_delay_ms = 5000;
  auth_data_recovery_backoff_policy_.multiply_factor = 2;
  auth_data_recovery_backoff_policy_.jitter_factor = 0.2;
  auth_data_recovery_backoff_policy_.maximum_backoff_ms = 1000 * 3600 * 4;
  auth_data_recovery_backoff_policy_.entry_lifetime_ms = -1;
  auth_data_recovery_backoff_policy_.always_use_initial_delay = false;
}

void SyncAuthKeeper::ScheduleAuthDataRecoveryRetry(
    AuthDataRecoveryRetryReason reason) {
  DCHECK(opera::SyncConfig::ShouldUseAuthTokenRecovery());
  DCHECK(!recovery_retry_timer_.IsRunning());
  DCHECK(!password_recoverer_);
  VLOG(1) << "Scheduling auth data recovery retry with reason "
          << AuthDataRecoveryRetryReasonToString(reason);
  auth_data_recovery_retry_reason_ = reason;
  auth_data_recovery_backoff_->InformOfRequest(false);
  const base::TimeDelta auth_data_recovery_delay =
      auth_data_recovery_backoff_->GetTimeUntilRelease();
  const base::TimeDelta zero_delay = base::TimeDelta();

  VLOG(1) << "Recovery retry delay is " << auth_data_recovery_delay;

  if (auth_data_recovery_delay == zero_delay) {
    // Should this ever happen, we'd rather loose the token than perform
    // immediate network requests against auth.
    // We need to send this in stats.
    VLOG(1) << "Recovery retry delay is zero, dropping retry request.";
    MarkTokenLost(TLR_BACKOFF_IN_THE_PAST);
    next_auth_data_recovery_time_ = base::Time();
  } else {
    next_auth_data_recovery_time_ =
        base::Time::Now() + auth_data_recovery_delay;
    recovery_retry_timer_.Start(FROM_HERE, auth_data_recovery_delay, this,
                                &SyncAuthKeeper::StartAuthDataRecovery);
    VLOG(1) << "Scheduled auth data recovery in " << auth_data_recovery_delay;
  }
}

void SyncAuthKeeper::MarkTokenLost(TokenLostReason reason) {
  DCHECK(SyncConfig::ShouldUseAuthTokenRecovery());
  VLOG(1) << "Token lost for '" << current_account_login() << "' ("
          << TokenLostReasonToString(reason) + ").";

  ChangeTokenLostReason(reason);
  ChangeTokenState(TokenState::TOKEN_STATE_LOST);
  auth_data_recovery_backoff_->InformOfRequest(false);

  UMA_HISTOGRAM_BOOLEAN("Opera.DSK.Sync.Recovery.Success", false);
  UMA_HISTOGRAM_ENUMERATION("Opera.DSK.Sync.Recovery.FailReason", reason,
                            TLR_LAST_ENUM_VALUE);

  MaybeNotifyStatusChanged();
}

void SyncAuthKeeper::NotifyInitializationComplete() {
  DCHECK(SyncConfig::ShouldUseAuthTokenRecovery());
  DCHECK(sync_service_);
  DCHECK(sync_service_->GetSyncClient());

  VLOG(1) << "Sync auth keeper initialization complete.";
  sync_service_->GetSyncClient()->OnSyncAuthKeeperInitialized(this);

  MaybeNotifyStatusChanged();
}

void SyncAuthKeeper::ResetRecoveryLimit() {
  recovery_limit_period_start_time_ = base::Time::Now();
  periodic_recovery_count_ = 0;
}

bool SyncAuthKeeper::AuthDataRecoveryLimitReached() {
  base::TimeDelta current_recovery_period =
      base::Time::Now() - recovery_limit_period_start_time_;
  DCHECK_GT(current_recovery_period, base::TimeDelta());
  if (current_recovery_period > kRecoveryLimitResetPeriod) {
    ResetRecoveryLimit();
  }
  return (periodic_recovery_count_ >= kRecoveryLimitCount);
}

void SyncAuthKeeper::UpdateLastSessionDescription(
    base::TimeDelta duration,
    const std::string& id,
    account_util::LogoutReason logout_reason) {
  last_session_description_.Update(duration, id, logout_reason);
}
base::TimeDelta SyncAuthKeeper::get_current_session_duration() const {
  DCHECK(account_);
  if (!account_->IsLoggedIn())
    return base::TimeDelta();

  if (account_->login_data().session_start_time == base::Time()) {
    return base::TimeDelta();
  }

  base::TimeDelta current_duration =
      base::Time::Now() - account_->login_data().session_start_time;
  current_duration = std::max(base::TimeDelta(), current_duration);
  return current_duration;
}

const std::string TokenLostReasonToString(
    SyncAuthKeeper::TokenLostReason reason) {
  switch (reason) {
    case SyncAuthKeeper::TLR_UNKNOWN:
      return "UNKNOWN";
    case SyncAuthKeeper::TLR_NO_AUTH_DATA_AFTER_RECOVERY:
      return "NO_AUTH_DATA_AFTER_RECOVERY";
    case SyncAuthKeeper::TLR_CREDENTIALS_NOT_FOUND:
      return "CREDENTIALS_NOT_FOUND";
    case SyncAuthKeeper::TLR_CREDENTIALS_INCORRECT:
      return "CREDENTIALS_INCORRECT";
    case SyncAuthKeeper::TLR_INTERNAL_ERROR:
      return "INTERNAL_ERROR";
    case SyncAuthKeeper::TLR_PASSWORD_MANAGER_TIMEOUT:
      return "PASSWORD_MANAGER_TIMEOUT";
    case SyncAuthKeeper::TLR_UNKNOWN_PASSWORD_RECOVERY_RESULT:
      return "UNKNOWN_PASSWORD_RECOVERY_RESULT";
    case SyncAuthKeeper::TLR_BACKOFF_IN_THE_PAST:
      return "BACKOFF_IN_THE_PAST";
    case SyncAuthKeeper::TLR_AUTH_ERROR_NOT_RECOGNIZED:
      return "AUTH_ERROR_NOT_RECOGNIZED";
    case SyncAuthKeeper::TLR_GOT_SIMPLE_AUTH_ERROR:
      return "GOT_SIMPLE_AUTH_ERROR";
    case SyncAuthKeeper::TLR_NO_PASSWORD_STORE:
      return "NO_PASSWORD_STORE";
    case SyncAuthKeeper::TLR_TOKEN_INVALID:
      return "TOKEN_INVALID";
    case SyncAuthKeeper::TLR_RECOVERY_LIMIT_REACHED:
      return "RECOVERY_LIMIT_REACHED";
    case SyncAuthKeeper::TLR_TOKEN_NOT_FOUND_ON_SERVER:
      return "TOKEN_NOT_FOUND_ON_SERVER";
    default:
      NOTREACHED();
      return "<UNKNOWN>";
  }
}

const std::string AuthDataRecoveryRetryReasonToString(
    SyncAuthKeeper::AuthDataRecoveryRetryReason reason) {
  switch (reason) {
    case SyncAuthKeeper::ADRRR_UNKNOWN:
      return "UNKNOWN";
    case SyncAuthKeeper::ADRRR_NETWORK_ERROR:
      return "NETWORK_ERROR";
    case SyncAuthKeeper::ADRRR_PARSE_ERROR:
      return "PARSE_ERROR";
    case SyncAuthKeeper::ADRRR_HTTP_ERROR:
      return "HTTP_ERROR";
    default:
      NOTREACHED();
      return "<UNKNOWN>";
  }
}

const std::string TokenStateToString(SyncAuthKeeper::TokenState state) {
  switch (state) {
    case SyncAuthKeeper::TOKEN_STATE_UNKNOWN:
      return "UNKNOWN";
    case SyncAuthKeeper::TOKEN_STATE_LOGGED_OUT:
      return "LOGGED_OUT";
    case SyncAuthKeeper::TOKEN_STATE_OK:
      return "OK";
    case SyncAuthKeeper::TOKEN_STATE_RECOVERING:
      return "RECOVERING";
    case SyncAuthKeeper::TOKEN_STATE_REFRESHING:
      return "REFRESHING";
    case SyncAuthKeeper::TOKEN_STATE_LOST:
      return "LOST";
    default:
      NOTREACHED();
      return "<UNKNOWN>";
  }
}
}  // namespace sync
}  // namespace opera
