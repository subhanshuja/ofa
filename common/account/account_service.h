// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_ACCOUNT_ACCOUNT_SERVICE_H_
#define COMMON_ACCOUNT_ACCOUNT_SERVICE_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "components/sync/util/opera_auth_problem.h"

#include "common/account/account_auth_data.h"
#include "common/account/account_util.h"

class GURL;

namespace opera {

class AccountObserver;
class AccountServiceDelegate;

/**
 * Provides services related to a (My)Opera account.
 *
 * It can be used to "sign" requests using the OAuth authorization scheme, see
 * GetSignedAuthHeader().
 *
 * It can be created and used on any thread but is not internally thread-safe.
 */
class AccountService {
 public:
  enum HttpMethod { HTTP_METHOD_GET, HTTP_METHOD_POST };

  /**
   * @param client_key the identifier portion of the client credentials.  A
   *    deprecated OAuth term for this is "Consumer Key".  Each Opera product
   *    is statically assigned a client key.
   * @param client_secret the client shared secret paired with @p client_key
   * @param delegate handles authorization token data
   */
  static AccountService* Create(const std::string& client_key,
                                const std::string& client_secret,
                                AccountServiceDelegate* delegate);

  AccountService();
  virtual ~AccountService();

  virtual void AddObserver(AccountObserver* observer) = 0;
  virtual void RemoveObserver(AccountObserver* observer) = 0;

  /**
   * Attempts to log the user in.
   *
   * This may start an asynchronous procedure to obtain an OAuth authorization
   * token.  Thus, the function must not be called while a previous procedure
   * is in progress.
   *
   * If the user is already logged in, AccountObservers are simply notified
   * about log-in success.
   *
   * AccountObservers may be notified about the result asynchronously, but on
   * the same thread that called this function.
   */
  virtual void LogIn() = 0;
  virtual void LogIn(opera_sync::OperaAuthProblem) = 0;

  /**
   * Logs the user out.
   *
   * It causes AccountService to discard the current OAuth authorization token.
   */
  virtual void LogOut(account_util::LogoutReason logout_reason) = 0;

  /**
   * Informs the AccountService that any current authorization token data known
   * to it is invalid.  This happens when the token is expired or discarded by
   * the authorization server.
   *
   */
  virtual void AuthDataExpired(opera_sync::OperaAuthProblem problem) = 0;

  /**
   * Obtains the payload for an HTTP Authorization header to be used to
   * authorize requests.
   *
   * Must only be called after logging in.  However, it is never guaranteed
   * that a request using the result of this function will be authorized,
   * because an OAuth authorization token can expire at any time.
   *
   * @param request_base_url the "base" URL of the request to be authorized,
   *    i.e., without any parameters.  Signing requests with parameters is
   *    currently not supported.
   * @param http_method the HTTP method of the request
   * @param realm the protection realm
   * @return the HTTP Authorization header payload
   */
  virtual std::string GetSignedAuthHeader(const GURL& request_base_url,
                                          HttpMethod http_method,
                                          const std::string& realm) const = 0;

  /**
   * Informs the AccountService that the current platform/device have a
   * system clock that differs too much from what the auth server is using.
   * All Auth requests will modify the local time with the given number
   * of seconds for the rest of the session.
   */
  // TODO(mzajaczkowski): Remove timeshift from here, this belongs in the auth
  // data.
  virtual void SetTimeSkew(int64_t time_skew) = 0;

  virtual bool HasAuthToken() const = 0;

  // Setup a custom retry timeout that is used for retrying the token requests
  // in case of temporary failure conditions (network error, parse error, HTTP
  // error). Note that the default for the given implementation will be used if
  // this is not called.
  virtual void SetRetryTimeout(const base::TimeDelta& delta) = 0;

  virtual base::Time next_token_request_time() const = 0;

  base::WeakPtr<AccountService> AsWeakPtr();

  virtual void OnAuthDataFetched(const AccountAuthData& auth_data,
                                 opera_sync::OperaAuthProblem problem) = 0;
  virtual void OnAuthDataFetchFailed(
      account_util::AuthDataUpdaterError error_code,
      account_util::AuthOperaComError auth_code,
      const std::string& message,
      opera_sync::OperaAuthProblem problem) = 0;
  virtual void DoRequestAuthData(opera_sync::OperaAuthProblem problem) = 0;

 private:
  base::WeakPtrFactory<AccountService> weak_ptr_factory_;
};

}  // namespace opera

#endif  // COMMON_ACCOUNT_ACCOUNT_SERVICE_H_
