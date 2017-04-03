// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_AUTH_KEEPER_H_
#define COMMON_SYNC_SYNC_AUTH_KEEPER_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "base/timer/timer.h"
#include "base/observer_list.h"
#include "components/keyed_service/core/keyed_service.h"
#include "net/base/backoff_entry.h"

#include "common/account/account_observer.h"
#include "common/sync/sync_auth_keeper_status.h"
#include "common/sync/sync_auth_keeper_util.h"
#include "common/sync/sync_password_recoverer.h"

class PrefService;

namespace net {
class URLRequestContextGetter;
}

namespace syncer {
class SyncService;
}

namespace opera {
namespace sync {
class SyncAccount;
class SyncAuthKeeperObserver;
class SyncAuthKeeperEventRecorder;

class SyncAuthKeeper : public AccountObserver, public KeyedService {
 public:
  enum TokenState {
    TOKEN_STATE_UNKNOWN,
    // The current known state of the account is "logged out"
    TOKEN_STATE_LOGGED_OUT,
    // The current known state of the account is "logged in".
    TOKEN_STATE_OK,
    // Waiting for password recoverer response, includes querying the
    // password store and performing a network request to the auth
    // service.
    TOKEN_STATE_RECOVERING,
    // Last known auth error is temporary (NET/HTTP/PARSE), we are expecting
    // the ProfileSyncService to retry the token request.
    TOKEN_STATE_REFRESHING,
    // This can happen for a number of reasons:
    // * The password recoverer claims a succesfull login with the recovered
    //   login data but no auth data was passed to us;
    // * The password recoverer failed to recover the login data or the auth
    //   data (no login data in password manager, wrong login data in password
    //   manager);
    // * The password store was not available for the password recoverer;
    // * The credentials found in the password manager were not correct, most
    //   probably the account password was changed on the server side or the
    //   account was removed.
    // * Token was explicitly marked as invalid during auth verification, we
    //   do not recover from this state.
    TOKEN_STATE_LOST,

    // Boundary.
    TOKEN_STATE_LAST_ENUM_VALUE
  };

  // No sorting on this enum. New values always just before
  // TLR_LAST_ENUM_VALUE. Remeber that this is sent as a
  // histogram.
  enum TokenLostReason {
    TLR_UNKNOWN,
    TLR_NO_AUTH_DATA_AFTER_RECOVERY,
    TLR_CREDENTIALS_NOT_FOUND,
    TLR_CREDENTIALS_INCORRECT,
    TLR_INTERNAL_ERROR,
    TLR_PASSWORD_MANAGER_TIMEOUT,
    TLR_UNKNOWN_PASSWORD_RECOVERY_RESULT,
    TLR_BACKOFF_IN_THE_PAST,
    TLR_AUTH_ERROR_NOT_RECOGNIZED,
    TLR_GOT_SIMPLE_AUTH_ERROR,
    TLR_GOT_NO_AUTH_ERROR,
    TLR_NO_PASSWORD_STORE,
    TLR_TOKEN_INVALID,
    TLR_RECOVERY_LIMIT_REACHED,
    TLR_TOKEN_NOT_FOUND_ON_SERVER,
    // Boundary, keep this last.
    TLR_LAST_ENUM_VALUE
  };

  enum AuthDataRecoveryRetryReason {
    ADRRR_UNKNOWN,
    ADRRR_HTTP_ERROR,
    ADRRR_PARSE_ERROR,
    ADRRR_NETWORK_ERROR,
    ADDRR_LAST_ENUM_VALUE
  };

  SyncAuthKeeper(syncer::SyncService* sync_service,
                 PrefService* pref_service);
  ~SyncAuthKeeper() override;

  void Initialize();

  // KeyedService implementation
  void Shutdown() override;

  // AccountObserver implementation
  void OnLoggedIn(AccountService* service,
                  opera_sync::OperaAuthProblem problem) override;
  void OnLoginError(AccountService* service,
                    account_util::AuthDataUpdaterError error_code,
                    account_util::AuthOperaComError auth_code,
                    const std::string& message,
                    opera_sync::OperaAuthProblem problem) override;
  void OnLoggedOut(AccountService* service,
                   account_util::LogoutReason logout_reason) override;
  void OnAuthDataExpired(AccountService* service,
                         opera_sync::OperaAuthProblem problem) override;
  void OnLoginRequested(AccountService* service,
                        opera_sync::OperaAuthProblem problem) override;
  void OnLogoutRequested(
      AccountService* service,
      opera::account_util::LogoutReason logout_reason) override;
  void OnLoginRequestDeferred(AccountService* service,
                              opera_sync::OperaAuthProblem problem,
                              base::Time scheduled_time) override;

  // Called back from SyncPasswordRecoverer
  void OnSyncPasswordRecoveryFinished(
      scoped_refptr<SyncPasswordRecoverer> recoverer,
      SyncPasswordRecoverer::Result result,
      const std::string& username,
      const std::string& password,
      const std::string& token,
      const std::string& token_secret);

  void AddObserver(SyncAuthKeeperObserver* observer);
  void RemoveObserver(SyncAuthKeeperObserver* observer);

  void MaybeNotifyStatusChanged();
  SyncAuthKeeperStatus status() const;

  TokenState token_state() const;
  std::string token_state_string() const;

  bool is_token_lost() const;

  SyncAuthKeeperEventRecorder* event_recorder() const;

 private:
  const base::TimeDelta kRecoveryLimitResetPeriod =
      base::TimeDelta::FromDays(7);
  const int kRecoveryLimitCount = 1000;

  void LoadPrefs();
  void SavePrefs();
  const std::string current_account_login() const;

  // Starts a new password/token/secret recovery attempt using the current
  // account login. Will drop a reference to any existing password recoverer,
  // effectively ignoring the result once available.
  void StartAuthDataRecovery();

  // Change the current token state and notify observers with
  // OnSyncAuthKeeperTokenStateChanged.
  void ChangeTokenState(TokenState token_state);

  void ChangeTokenLostReason(TokenLostReason reason);

  // Taking the given auth code into the account either ignore the call
  void OnAuthError(account_util::AuthOperaComError auth_code);

  void InitAuthDataRecoveryBackoffPolicy();

  void ScheduleAuthDataRecoveryRetry(AuthDataRecoveryRetryReason reason);

  void MarkTokenLost(TokenLostReason reason);

  void NotifyInitializationComplete();

  void ResetRecoveryLimit();
  bool AuthDataRecoveryLimitReached();

  void UpdateLastSessionDescription(base::TimeDelta duration,
                                    const std::string& id,
                                    account_util::LogoutReason logout_reason);

  base::TimeDelta get_current_session_duration() const;

  // The SyncAccount that drives Opera sync for the given profile.
  opera::SyncAccount* account_;

  // The backoff entry that determines auth data recovery retry delay.
  std::unique_ptr<net::BackoffEntry> auth_data_recovery_backoff_;

  // The backoff policy for auth data recovery retry (5 s - 4 h currently).
  net::BackoffEntry::Policy auth_data_recovery_backoff_policy_;

  // Auth data recovery retry reason that will only be available while the
  // keeper is in recovery retry state. For account-internals.
  AuthDataRecoveryRetryReason auth_data_recovery_retry_reason_;

  // Holds the state of the last sync session. It is filled with new data on
  // logout. Contents is preserved across restarts. Used for account-internals.
  LastSessionDescription last_session_description_;

  // Preserves the scheduled time of the next token recovery attempt
  // for displaying in account-internals. Token recovery attempts will
  // be repeated in case of temporary errors on an attempt to fetch
  // auth data via the simple login endpoint (HTTP error, NETWORK error,
  // PARSE error).
  base::Time next_auth_data_recovery_time_;

  // Observers. Remember to register as SyncAuthKeeper observer and wait
  // for forwarded events from SyncAccountService rather than registering
  // as SyncAccountService observer.
  base::ObserverList<SyncAuthKeeperObserver> observers_;

  // The currently operating SyncPasswordRecoverer. Note that since
  // the SyncPasswordRecoverer can't really be interrupted in its
  // current implementation and since it will delete itself after it
  // finishes operation, we only hold a reference to the recoverer
  // instance that we have started as the last one. All callback from
  // previous password recovery attempts will be ignored.
  scoped_refptr<SyncPasswordRecoverer> password_recoverer_;

  // The token recovery count within the current recovery limit period.
  // Recovery will not be performed in case this count exceeds
  // kRecoveryLimitCount. Note that this counter will be reset on each
  // client restart and also after within the end of recovery period
  // compare kRecoveryLimitResetPeriod.
  int periodic_recovery_count_;

  // Pref service for profile.
  PrefService* pref_service_;

  // Holds the start time of the current recovery count reset period.
  base::Time recovery_limit_period_start_time_;

  // Auth data recovery retry timer.
  base::OneShotTimer recovery_retry_timer_;

  // The ProfileSyncService for the profile.
  syncer::SyncService* sync_service_;

  // Last reason for losing the auth token.
  TokenLostReason token_lost_reason_;

  // Holds information between the "auth data expired" and "error"/"ok" events
  // for auth data fetch procedure. Used in account-internals.
  TokenRefreshDescription token_refresh_description_;

  // The current token state. Will be preserved accross browser restarts.
  TokenState token_state_;

  SyncAuthKeeperStatus prev_status_;

  std::unique_ptr<SyncAuthKeeperEventRecorder> event_recorder_;

  bool initialized_;

  base::WeakPtrFactory<SyncAuthKeeper> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SyncAuthKeeper);
};

const std::string TokenStateToString(SyncAuthKeeper::TokenState token_state);
const std::string TokenLostReasonToString(
    SyncAuthKeeper::TokenLostReason reason);
const std::string AuthDataRecoveryRetryReasonToString(
    SyncAuthKeeper::AuthDataRecoveryRetryReason reason);
}  // namespace sync
}  // namespace opera
#endif  // COMMON_SYNC_SYNC_AUTH_KEEPER_H_
