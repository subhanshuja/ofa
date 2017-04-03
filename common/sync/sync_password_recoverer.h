// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_PASSWORD_RECOVERER_H_
#define COMMON_SYNC_SYNC_PASSWORD_RECOVERER_H_

#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/timer/timer.h"
#include "chrome/browser/password_manager/password_store_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/password_manager/core/browser/password_store.h"
#include "components/password_manager/core/browser/password_store_consumer.h"

#include "common/account/account_observer.h"
#include "common/account/account_service.h"
#include "common/sync/sync_account.h"
#include "common/sync/sync_auth_data_updater.h"

using autofill::PasswordForm;

namespace opera {
class SyncPasswordRecoverer : public opera::AccountObserver,
                              public password_manager::PasswordStoreConsumer,
                              public base::RefCounted<SyncPasswordRecoverer> {
 public:
  enum Result {
    SPRR_UNKNOWN,
    // Password and auth data fetched correctly.
    SPRR_RECOVERED,
    // Could not obtain the password store for profile.
    SPRR_INTERNAL_ERROR,
    // Credentials were not found in the password manager.
    SPRR_CREDENTIALS_NOT_FOUND,
    // The auth service did verify te credentials to be incorrect.
    SPRR_CREDENTIALS_INCORRECT,
    // Retryable failure, auth service returned an HTTP error.
    SPRR_HTTP_ERROR,
    // Retryable failure, network error while contacting auth service.
    SPRR_NETWORK_ERROR,
    // Retryable failure, auth service response could not be parsed.
    SPRR_PARSE_ERROR,
    // Timeout waiting for password manager.
    SPRR_PASSWORD_MANAGER_TIMEOUT,
  };

  typedef base::Callback<void(scoped_refptr<SyncPasswordRecoverer> recoverer,
                              Result result,
                              const std::string& username,
                              const std::string& password,
                              const std::string& token,
                              const std::string& token_secret)>
      RecoveryFinishedCallback;

  explicit SyncPasswordRecoverer(Profile* profile);
  bool TryRecover(const std::string& sync_username,
                  RecoveryFinishedCallback finished_callback);

  bool is_idle() const { return state_ == IDLE; }
  Result result() const { return result_; }

  std::string user_name() const;

  void OnPasswordStoreRequestTimeout();

  // PasswordStoreConsumer
  void OnGetPasswordStoreResults(
      std::vector<std::unique_ptr<autofill::PasswordForm>> results) override;

  // AccountObserver
  void OnLoggedIn(opera::AccountService* service,
                  opera_sync::OperaAuthProblem problem) override;
  void OnLoginError(opera::AccountService* service,
                    opera::account_util::AuthDataUpdaterError error_code,
                    opera::account_util::AuthOperaComError auth_code,
                    const std::string& message,
                    opera_sync::OperaAuthProblem problem) override;
  void OnLoggedOut(opera::AccountService* service,
                   account_util::LogoutReason logout_reason) override {}
  void OnAuthDataExpired(opera::AccountService* service,
                         opera_sync::OperaAuthProblem problem) override {}
  void OnLoginRequested(opera::AccountService* service,
                        opera_sync::OperaAuthProblem problem) override {}
  void OnLogoutRequested(
      opera::AccountService* service,
      opera::account_util::LogoutReason logout_reason) override {}

  void OverridePasswordStoreTimeoutForTest(int new_timeout_ms);

 private:
  enum State { IDLE, WAIT_FOR_PASSWORD_MANAGER, WAIT_FOR_ACCOUNT };

  ~SyncPasswordRecoverer() override;
  friend class base::RefCounted<SyncPasswordRecoverer>;

  void TryLogin();
  void Notify();

  std::string PathToAuthURL(const std::string& path);

  void OnPostNotify();

  State state_;
  Result result_;
  Profile* profile_;
  RecoveryFinishedCallback finished_callback_;

  int password_store_timeout_ms_;
  base::OneShotTimer timer_;

  std::string password_;
  base::string16 sync_username_;
  std::string token_;
  std::string token_secret_;

  std::unique_ptr<opera::SyncAccount> recoverer_account_;
  scoped_refptr<password_manager::PasswordStore> password_store_;
};

const std::string SyncPasswordRecovererResultToString(
    SyncPasswordRecoverer::Result result);
}  // namespace opera

#endif  // COMMON_SYNC_SYNC_PASSWORD_RECOVERER_H_
