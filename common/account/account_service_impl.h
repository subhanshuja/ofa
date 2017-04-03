// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_ACCOUNT_ACCOUNT_SERVICE_IMPL_H_
#define COMMON_ACCOUNT_ACCOUNT_SERVICE_IMPL_H_

#include <string>

#include "base/observer_list.h"
#include "base/timer/timer.h"
#include "base/threading/non_thread_safe.h"

#include "common/account/account_service.h"
#include "common/account/account_util.h"

namespace opera {

/**
 * The default implementation of AccountService.
 */
class AccountServiceImpl : public AccountService, public base::NonThreadSafe {
 public:
  /**
   * @see AccountService::Create()
   */
  AccountServiceImpl(const std::string& client_key,
                     const std::string& client_secret,
                     AccountServiceDelegate* delegate);
  ~AccountServiceImpl() override;

  /**
   * @name AccountService implementation
   * @{
   */
  void AddObserver(AccountObserver* observer) override;
  void RemoveObserver(AccountObserver* observer) override;

  void LogIn() override;
  void LogIn(opera_sync::OperaAuthProblem) override;
  void LogOut(account_util::LogoutReason logout_reason) override;
  void AuthDataExpired(opera_sync::OperaAuthProblem problem) override;

  std::string GetSignedAuthHeader(const GURL& request_base_url,
                                  HttpMethod http_method,
                                  const std::string& realm) const override;

  void SetTimeSkew(int64_t time_skew) override;

  /** @} */

  /**
   * Obtains the payload for an HTTP Authorization header to be used to
   * authorize requests.
   *
   * This is like GetSignedAuthHeader(), only it uses the given timestamp and
   * nonce values rather than generating them automatically.
   */
  std::string GetSignedAuthHeaderWithTimestamp(const GURL& request_base_url,
                                               HttpMethod http_method,
                                               const std::string& realm,
                                               const std::string& timestamp,
                                               const std::string& nonce) const;

 private:
  void OnAuthDataFetched(const AccountAuthData& auth_data,
                         opera_sync::OperaAuthProblem problem) override;
  void OnAuthDataFetchFailed(account_util::AuthDataUpdaterError error_code,
                             account_util::AuthOperaComError auth_code,
                             const std::string& message,
                             opera_sync::OperaAuthProblem problem) override;

  bool HasAuthToken() const override;

  base::Time next_token_request_time() const override;

  bool NotifyIfLoggedIn(opera_sync::OperaAuthProblem problem);

  const std::string client_key_;
  const std::string client_secret_;

  // In case opera::SyncConfig::ShouldUseAuthTokenRecovery() is
  // enabled we enable auth HTTP request throttling that is supposed
  // to protect us against hammering the remote auth service when there
  // is any kind of client and/or server malfunction that results in
  // a token renewal loop.
  // Note that we can fall into token renewal loop in case
  // we get constant auth error at the verification endpoint
  // while at the same time the token renewal endpoint happily
  // returns new tokens.
  // This was only reproducible with the testing server so far
  // however this should be fixed anyway.
  // The idea here is to only allow a token renewal request each
  // kMinTokenRenewalDelta (e.g. each 60 seconds).

  // This is the method that actually trigger the auth data request.
  void DoRequestAuthData(opera_sync::OperaAuthProblem problem) override;

  // Whether we should trigger the request immediatelly or wait.
  bool ShouldDelayTokenRequest() const;

  // Time until the next request is allowed by policy.
  base::TimeDelta DeltaTillNextTokenRequestAllowed() const;

  void SetRetryTimeout(const base::TimeDelta& delta) override;

  // Default minimal delta between two token renewal requests are
  // allowed.
  const base::TimeDelta kDefaultRetryTimeout = base::TimeDelta::FromMinutes(1);

  // The time when the last token request was SENT (note that this
  // will differ from the time when get the response).
  base::Time last_token_request_time_;

  // The time of the next scheduled token request, for debug UI mainly.
  base::Time next_token_request_time_;

  // Token request delay timer.
  base::OneShotTimer request_token_timer_;

  int64_t time_skew_;

  AccountAuthData auth_data_;

  AccountServiceDelegate* delegate_;

  // Current retry timeout for temporary failures, defaults to
  // kDefaultRetryTimeout.
  base::TimeDelta retry_timeout_;

  base::ObserverList<AccountObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(AccountServiceImpl);
};

}  // namespace opera

#endif  // COMMON_ACCOUNT_ACCOUNT_SERVICE_IMPL_H_
