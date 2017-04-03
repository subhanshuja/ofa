// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_ACCOUNT_H_
#define COMMON_SYNC_SYNC_ACCOUNT_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/memory/weak_ptr.h"

#include "common/account/account_util.h"
#include "components/sync/util/opera_auth_problem.h"

namespace net {
class URLRequestContextGetter;
}

namespace opera {

struct AccountAuthData;
class AccountAuthDataFetcher;
class AccountService;
class AccountServiceDelegate;
class SyncLoginDataStore;
struct SyncLoginData;
struct SyncLoginErrorData;
class TimeSkewResolver;

class ExternalCredentials {
 public:
  ExternalCredentials() {}
  ExternalCredentials(const std::string& username, const std::string& password):
    username_(username), password_(password) {}
  bool Empty() const { return (username_.empty() || password_.empty()); }
  std::string username() const { return username_; }
  std::string password() const { return password_; }

 private:
  std::string username_;
  std::string password_;
};

/**
 * Represents an AccountService-based user account used by Sync.
 */
class SyncAccount {
 public:
  typedef base::Callback<AccountService*(AccountServiceDelegate* delegate)>
      ServiceFactory;
  typedef base::Callback<AccountAuthDataFetcher*(
      const AccountAuthData& old_data)> AuthDataUpdaterFactory;
  virtual ~SyncAccount() {}

  static std::unique_ptr<SyncAccount> Create(
      net::URLRequestContextGetter* url_context_getter,
      const ServiceFactory& service_factory,
      std::unique_ptr<TimeSkewResolver> time_skew_resolver,
      std::unique_ptr<SyncLoginDataStore> login_data_store,
      const AuthDataUpdaterFactory& auth_data_updater_factory,
      const base::Closure& stop_syncing_callback,
      const ExternalCredentials& external_credentials);

  virtual base::WeakPtr<AccountService> service() const = 0;
  virtual void SetLoginData(const SyncLoginData& login_data) = 0;
  virtual const SyncLoginData& login_data() const = 0;
  virtual void SetLoginErrorData(
      const SyncLoginErrorData& login_error_data) = 0;
  virtual const SyncLoginErrorData& login_error_data() const = 0;
  /**
   * Checks whether the account has the login data.
   */
  virtual bool HasAuthData() const = 0;
  /**
   * Checks whether the account has the login data and it's valid.
   */
  virtual bool HasValidAuthData() const = 0;

  bool IsLoggedIn() const { return HasAuthData(); }

  virtual void LogIn();

  /**
   * Sets the login/auth data directly, when these are available from some
   * other source on the platform, i.e. the UI login page on desktop.
   * The |login_data| needs to contain at least a non-empty username and
   * valid auth data.
   * Will result in a call OnLoggedIn().
   */
  virtual void LogInWithAuthData(const SyncLoginData& login_data);
  virtual void LogOut(account_util::LogoutReason logout_reason);

  /**
   * Signals the account that any mechanism using the auth data was told that
   * the auth data has expired.
   * The result is an auth data refresh and a callback to
   * OnLoggedIn()/OnLoginError().
   * If there was a previous call to this method, that did mark the auth data as
   * expired already, any further call will return immediately until the auth
   * data is available.
   */
  virtual void AuthDataExpired(opera_sync::OperaAuthProblem);

  /**
   * Force a retry of auth data renewal, even if the data is already marked as
   * expired.
   * Note that you can only call this if the previous attempt to renew the
   * auth data failed for network/infrastructure/whatever conditions, other
   * than AUTH_ERROR.
   */
  virtual void RetryAuthDataRenewal(opera_sync::OperaAuthProblem);

  /**
   * Informs SyncAccount that Sync has become enabled.
   */
  virtual void SyncEnabled() = 0;
};

}  // namespace opera

#endif  // COMMON_SYNC_SYNC_ACCOUNT_H_
