// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include <vector>

#include "common/sync/sync_account_impl.h"

#include "base/bind.h"
#include "base/guid.h"
#include "base/logging.h"
#include "chrome/browser/profiles/profile.h"

#include "common/account/account_service.h"
#include "common/account/account_service_delegate.h"
#include "common/account/account_util.h"
#include "common/account/time_skew_resolver.h"
#include "common/sync/sync_auth_data_updater.h"
#include "common/sync/sync_config.h"
#include "common/sync/sync_login_data_store.h"
#include "common/sync/sync_server_info.h"

#if defined(OPERA_DESKTOP)
#include "common/account/oauth_token_fetcher_impl.h"
#endif  // OPERA_DESKTOP

namespace opera {

namespace {

// It is possible that we'll be sending out token renewal requests
// while still having the valid token. In that case we'll be getting
// the 428 error code from auth.opera.com and we will be moving forward
// with the same token we had.
// In case that the Chromium sync engine is changed in upstream so that
// it refuses further operation in case the token does not change after
// signalling its expiration and continues to request the renewal, we
// might fall into an endless loop of token renewals.
// This is the last resort to break this loop, bailing out with what
// we consider an auth error.
const uint16_t kMaxTokenNotChangedCount = 5;

}  // namespace

// static
std::unique_ptr<SyncAccount> SyncAccount::Create(
    net::URLRequestContextGetter* url_context_getter,
    const SyncAccount::ServiceFactory& service_factory,
    std::unique_ptr<TimeSkewResolver> time_skew_resolver,
    std::unique_ptr<SyncLoginDataStore> login_data_store,
    const AuthDataUpdaterFactory& auth_data_updater_factory,
    const base::Closure& stop_syncing_callback,
    const opera::ExternalCredentials& external_credentials) {
  std::unique_ptr<SyncAccount> account(new SyncAccountImpl(
      service_factory, std::move(time_skew_resolver),
      std::move(login_data_store), url_context_getter,
      auth_data_updater_factory, stop_syncing_callback, external_credentials));

  return account;
}

class SyncAccountImpl::ServiceDelegate : public AccountServiceDelegate {
 public:
  ServiceDelegate(SyncAccountImpl* account,
                  const AuthDataUpdaterFactory& auth_data_updater_factory,
                  std::unique_ptr<TimeSkewResolver> time_skew_resolver,
                  const base::Closure& stop_syncing_callback)
      : account_(account),
        auth_data_updater_factory_(auth_data_updater_factory),
        time_skew_resolver_(std::move(time_skew_resolver)),
        stop_syncing_callback_(stop_syncing_callback),
        token_not_changed_count_(0) {}

  /**
   * @name AccountServiceDelegate implementation
   * @{
   */
  void RequestAuthData(
      opera_sync::OperaAuthProblem problem,
      const RequestAuthDataSuccessCallback& success_callback,
      const RequestAuthDataFailureCallback& failure_callback) override {
    // If the callback is set it means the token is already being refreshed.
    // It may happen because couple of services might require token refresh
    // e.g. ProfileSyncService and TiclInvalidationService.

    VLOG(4) << "Authorization data request";

    if (!success_callback_.is_null()) {
      VLOG(4) << "Another authorization data request in progress";
      DCHECK(!failure_callback_.is_null());
      return;
    }

    current_problem_ = problem;

    if (!account_->login_data_.has_auth_data()) {
      // TODO(mzajaczkowski): We don't have the previous auth data while
      // trying to renew it. This is quite a problem since we do need the
      // previous auth token for renewal.
      // Why would this happen anyway?
      VLOG(2) << "No auth data for renewal.";
      if (!SyncConfig::ShouldUseAuthTokenRecovery()) {
        stop_syncing_callback_.Run();
      }
      failure_callback.Run(account_util::ADUE_INTERNAL_ERROR,
                           account_util::AOCE_NO_ERROR,
                           "Auth data lost in flight.", current_problem_);
    } else if (account_->auth_data_valid_) {
      VLOG(1) << "Authorization data already available";
      success_callback.Run(account_->login_data_.auth_data, current_problem_);
    } else if (!opera::SyncConfig().ShouldUseSmartTokenHandling() &&
               token_not_changed_count_ >= kMaxTokenNotChangedCount) {
      VLOG(2) << "Valid token refresh";
      // TODO(mzajaczkowski): Is this an internal error instead? What is this?
      failure_callback.Run(
          account_util::ADUE_AUTH_ERROR,
          account_util::AOCE_428_OPERA_ACCESS_TOKEN_NOT_EXPIRED,
          "Token expiration failure", current_problem_);
    } else {
      VLOG(2) << "Authorization data request performed";
      success_callback_ = success_callback;
      failure_callback_ = failure_callback;

      DCHECK(auth_data_updater_ == nullptr);
      auth_data_updater_.reset(
          auth_data_updater_factory_.Run(account_->login_data_.auth_data));
      if (time_skew_resolver_) {
        time_skew_resolver_->RequestTimeSkew(base::Bind(
            &ServiceDelegate::TimeSkewFetched, base::Unretained(this)));
      } else {
        UpdateAuthData();
      }
    }
  }

  void InvalidateAuthData() override {
    VLOG(1) << "Authorization data expired";
    account_->auth_data_valid_ = false;
  }

  void LoggedOut() override { account_->LoggedOut(); }
  /** @} */

 private:
  void TimeSkewFetched(const TimeSkewResolver::ResultValue& value) {
    if (SetTimeSkew(value)) {
      // Time skew changed - we assume it was time skew fault and go on
      AuthDataUpdated(account_->login_data_.auth_data, current_problem_);
    } else {
      // Time skew didn't change (or fetch failed) - try to update auth data
      // TODO(mzajaczkowski): Fix this.
      UpdateAuthData();
    }
  }

  void UpdateAuthData() {
    if (opera::SyncConfig().ShouldUseSmartTokenHandling() &&
        WasTokenExchanged(current_problem_.token())) {
      VLOG(1) << current_problem_.ToString()
              << " token already exchanged, responding with the current token.";
      AuthDataUpdated(account_->login_data().auth_data, current_problem_);
    } else {
      auth_data_updater_->RequestAuthData(
          current_problem_,
          base::Bind(&ServiceDelegate::AuthDataUpdated, base::Unretained(this)),
          base::Bind(&ServiceDelegate::AuthDataUpdateFailed,
                     base::Unretained(this)));
    }
  }

  void AuthDataUpdated(const AccountAuthData& new_auth_data,
                       opera_sync::OperaAuthProblem problem) {
    DCHECK(!new_auth_data.token.empty());
    DCHECK(!new_auth_data.token_secret.empty());

    VLOG(1) << "Authorization data obtained " << new_auth_data.token;

    if (opera::SyncConfig().ShouldUseSmartTokenHandling()) {
      const std::string& old_token = current_problem_.token();
      if (old_token != new_auth_data.token) {
        // This is a new token, mark the previous one as exchanged.
        MarkTokenExchanged(old_token);
      }
    } else {
      if (account_->login_data().auth_data.token == new_auth_data.token)
        token_not_changed_count_++;
      else
        token_not_changed_count_ = 0;
    }

    SyncLoginData new_data(account_->login_data());
    new_data.auth_data = new_auth_data;
    account_->SetLoginData(new_data);

    failure_callback_.Reset();
    success_callback_.Run(new_auth_data, problem);
    success_callback_.Reset();

    auth_data_updater_.reset();

    // Save the newly received auth data so that if sync or the entire browser
    // gets restarted, we'll reconnect with a valid token.
    account_->login_data_store_->SaveLoginData(new_data);
  }

  void AuthDataUpdateFailed(account_util::AuthDataUpdaterError error_code,
                            account_util::AuthOperaComError auth_code,
                            const std::string& message,
                            opera_sync::OperaAuthProblem problem) {
    VLOG(1) << "Authorization data request problem: "
            << "(" << account_util::AuthDataUpdaterErrorToString(error_code)
            << ") (" << account_util::AuthOperaComErrorToString(auth_code)
            << ") (" << problem.ToString() << ")'" << message << "'";

    success_callback_.Reset();
    auth_data_updater_.reset();

    if (!failure_callback_.is_null()) {
      failure_callback_.Run(error_code, auth_code, message, problem);
      failure_callback_.Reset();
    }
  }

  bool WasTokenExchanged(const std::string& token) {
    DCHECK(opera::SyncConfig().ShouldUseSmartTokenHandling());
    for (auto it = exchanged_tokens_.begin(); it != exchanged_tokens_.end();
         it++) {
      if (*it == token) {
        VLOG(1) << "Token '" << token << "' was already exchanged.";
        return true;
      }
    }
    VLOG(1) << "Token '" << token << "' was NOT yet exchanged!";
    return false;
  }

  void MarkTokenExchanged(const std::string& token) {
    DCHECK(opera::SyncConfig().ShouldUseSmartTokenHandling());
    const int kMaxTokenCount = 5;

    if (WasTokenExchanged(token))
      return;

    if (exchanged_tokens_.size() > kMaxTokenCount)
      exchanged_tokens_.erase(exchanged_tokens_.begin());

    exchanged_tokens_.push_back(token);

    VLOG(1) << "Token '" << token << "' marked as exchanged. Now have "
            << exchanged_tokens_.size() << " old tokens.";
  }

  bool SetTimeSkew(const TimeSkewResolver::ResultValue& value) {
    int64_t new_time_skew = 0;
    bool time_skew_fetched = false;

    switch (value.result) {
      case TimeSkewResolver::QueryResult::INVALID_RESPONSE_ERROR:
        // Server side error - try to ignore and move on as if everything was OK
        // TODO(mzajaczkowski): Fix this.
        LOG(WARNING) << "Obtained unexpected time skew value from server: "
                     << value.error_message;
      case TimeSkewResolver::QueryResult::TIME_OK: {
        new_time_skew = 0;
        time_skew_fetched = true;
      } break;

      case TimeSkewResolver::QueryResult::TIME_SKEW: {
        new_time_skew = -value.time_skew;
        time_skew_fetched = true;
      } break;

      case TimeSkewResolver::QueryResult::BAD_REQUEST_ERROR:
      case TimeSkewResolver::QueryResult::NO_RESPONSE_ERROR: {
        // Error on our side - we should fail auth request
        new_time_skew = 0;
        time_skew_fetched = false;
        LOG(ERROR) << "Failed to obtain time skew value from server: "
                   << value.error_message;
      } break;

      default: {
        // In case someone forgot to add new case, treat that like an error
        NOTREACHED();
      } break;
    }

    bool time_skew_changed = account_->login_data_.time_skew != new_time_skew;

    if (time_skew_fetched && time_skew_changed) {
      account_->login_data_.time_skew = new_time_skew;
      account_->account_service_->SetTimeSkew(new_time_skew);
      return true;
    }
    return false;
  }

  SyncAccountImpl* account_;

  const AuthDataUpdaterFactory auth_data_updater_factory_;
  std::unique_ptr<AccountAuthDataFetcher> auth_data_updater_;
  std::unique_ptr<TimeSkewResolver> time_skew_resolver_;
  AccountAuthDataFetcher::RequestAuthDataSuccessCallback success_callback_;
  AccountAuthDataFetcher::RequestAuthDataFailureCallback failure_callback_;

  const base::Closure stop_syncing_callback_;

  uint16_t token_not_changed_count_;

  std::vector<std::string> exchanged_tokens_;
  opera_sync::OperaAuthProblem current_problem_;
};

#if defined(OPERA_DESKTOP)
class SyncAccountImpl::CredentialsBasedServiceDelegate
    : public AccountServiceDelegate {
 public:
  CredentialsBasedServiceDelegate(
      SyncAccountImpl* account,
      const std::string& auth_server_url,
      net::URLRequestContextGetter* request_context_getter,
      const std::string& user_name,
      const std::string& password)
      : account_(account) {
    OAuthTokenFetcher* fetcher = new OAuthTokenFetcherImpl(
        auth_server_url, SyncConfig::ClientKey(), request_context_getter);
    fetcher->SetUserCredentials(user_name, password);
    fetcher_.reset(fetcher);
  }

  /**
   * @name AccountServiceDelegate implementation
   * @{
   */
  void RequestAuthData(
      opera_sync::OperaAuthProblem problem,
      const RequestAuthDataSuccessCallback& success_callback,
      const RequestAuthDataFailureCallback& failure_callback) override {
    // If the callback is set it means the token is already being refreshed.
    // It may happen because couple of services might require token renewal
    // e.g. ProfileSyncService and TiclInvalidationService.
    if (!success_callback_.is_null())
      return;

    success_callback_ = success_callback;
    fetcher_->RequestAuthData(
        problem, base::Bind(&CredentialsBasedServiceDelegate::AuthDataUpdated,
                            base::Unretained(this)),
        failure_callback);
  }

  void InvalidateAuthData() override {}

  void LoggedOut() override {}

  /** @} */

 private:
  void AuthDataUpdated(const AccountAuthData& new_auth_data,
                       opera_sync::OperaAuthProblem problem) {
    DCHECK(!new_auth_data.token.empty());
    DCHECK(!new_auth_data.token_secret.empty());
    SyncLoginData new_data(account_->login_data());
    new_data.auth_data = new_auth_data;
    account_->SetLoginData(new_data);

    success_callback_.Run(new_auth_data, problem);
    success_callback_.Reset();
  }

  SyncAccountImpl* account_;
  AccountAuthDataFetcher::RequestAuthDataSuccessCallback success_callback_;
  std::unique_ptr<OAuthTokenFetcher> fetcher_;
};
#endif  // OPERA_DESKTOP

SyncAccountImpl::SyncAccountImpl(
    const ServiceFactory& service_factory,
    std::unique_ptr<TimeSkewResolver> time_skew_resolver,
    std::unique_ptr<SyncLoginDataStore> login_data_store,
    net::URLRequestContextGetter* request_context_getter,
    const AuthDataUpdaterFactory& auth_data_updater_factory,
    const base::Closure& stop_syncing_callback,
    const opera::ExternalCredentials& external_credentials)
    : login_data_store_(std::move(login_data_store)), auth_data_valid_(true) {
  // If user credentials were specified, use that first to log in.
  std::string user_name;
  std::string password;
  if (!external_credentials.Empty()) {
#if defined(OPERA_DESKTOP)
    if (!user_name.empty() || !password.empty()) {
      VLOG(1) << "Overriding command line with external.";
    }
    user_name = external_credentials.username();
    password = external_credentials.password();

    const std::string simple_token_request_url =
        SyncConfig::AuthServerURL().spec() +
        "service/oauth/simple/access_token";
    account_service_delegate_.reset(
        // Will this delegate ever refresh the token? Include timeskew
        // offset if needed.
        new CredentialsBasedServiceDelegate(this, simple_token_request_url,
                                            request_context_getter, user_name,
                                            password));
    login_data_.used_username_to_login =
        user_name.find('@') == std::string::npos;
    if (login_data_.used_username_to_login)
      login_data_.user_name = user_name;
    else
      login_data_.user_email = user_name;
    login_data_.password = password;
#endif  // OPERA_DESKTOP
  } else {
    account_service_delegate_.reset(
        new ServiceDelegate(this, auth_data_updater_factory,
                            std::move(time_skew_resolver),
                            stop_syncing_callback));
    login_data_ = login_data_store_->LoadLoginData();
  }
  if (login_data_.session_id.empty())
    login_data_.session_id = base::GenerateGUID();

  account_service_.reset(service_factory.Run(account_service_delegate_.get()));
}

SyncAccountImpl::~SyncAccountImpl() {}

base::WeakPtr<AccountService> SyncAccountImpl::service() const {
  return account_service_->AsWeakPtr();
}

const SyncLoginData& SyncAccountImpl::login_data() const {
  return login_data_;
}

const SyncLoginErrorData& SyncAccountImpl::login_error_data() const {
  return login_error_data_;
}

bool SyncAccountImpl::HasAuthData() const {
  return login_data_.has_auth_data();
}

bool SyncAccountImpl::HasValidAuthData() const {
  return HasAuthData() && auth_data_valid_;
}

void SyncAccountImpl::SetLoginData(const SyncLoginData& login_data) {
  DCHECK(!login_data.display_name().empty());

  login_data_ = login_data;
  auth_data_valid_ = true;
}

void SyncAccountImpl::SetLoginErrorData(
    const SyncLoginErrorData& login_error_data) {
  login_error_data_ = login_error_data;
  LoggedOut();
}

void SyncAccountImpl::SyncEnabled() {
  DCHECK(!login_data_.session_id.empty());

  // Don't store the auth data when credentials come from CMD.
  login_data_store_->SaveLoginData(login_data_);
}

void SyncAccountImpl::LoggedOut() {
  if (SyncConfig::ShouldUseAutoPasswordRecovery()) {
    login_data_ = SyncLoginData();
  }
  login_data_.auth_data = AccountAuthData();
  login_data_store_->ClearSessionAndTokenData();
}

}  // namespace opera
