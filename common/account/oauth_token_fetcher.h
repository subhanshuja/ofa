// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_ACCOUNT_OAUTH_TOKEN_FETCHER_H_
#define COMMON_ACCOUNT_OAUTH_TOKEN_FETCHER_H_

#include <string>

#include "common/account/account_service_delegate.h"

namespace opera {

/**
 * A credentials-based AccountAuthDataFetcher.
 *
 * This fetches OAuth authorization tokens using the client side of the "Access
 * token simple request" specified for Opera's centralized authentication
 * system.
 *
 * Typically for OAuth, the authorization token is obtained in three steps, as
 * described in http://tools.ietf.org/html/rfc5849#section-2.  The "simple
 * request" implemented here requires just one step: it is assumed the
 * resource owner (Opera user) authorizes the client (Opera client software)
 * to access the resources on the server in one step.
 *
 * This class may be used on any thread, but it's not designed for concurrent
 * access.
 */
class OAuthTokenFetcher : public AccountAuthDataFetcher {
 public:
  ~OAuthTokenFetcher() override;

  void SetUserCredentials(const std::string& user_name,
                          const std::string& password);

  /**
   * @name AccountServiceDelegate implementation
   * @{
   */
  void RequestAuthData(
      opera_sync::OperaAuthProblem problem,
      const RequestAuthDataSuccessCallback& success_callback,
      const RequestAuthDataFailureCallback& failure_callback) override;
  /** @} */

 protected:
  OAuthTokenFetcher();

  /**
   * Starts fetching the OAuth authorization token for the user with the
   * currently set credentials.
   */
  virtual void Start() = 0;

  std::string user_name_;
  std::string password_;
  RequestAuthDataSuccessCallback success_callback_;
  RequestAuthDataFailureCallback failure_callback_;

  DISALLOW_COPY_AND_ASSIGN(OAuthTokenFetcher);
};

}  // namespace opera

#endif  // COMMON_ACCOUNT_OAUTH_TOKEN_FETCHER_H_
