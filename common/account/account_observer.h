// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_ACCOUNT_ACCOUNT_OBSERVER_H_
#define COMMON_ACCOUNT_ACCOUNT_OBSERVER_H_

#include <string>

#include "components/sync/util/opera_auth_problem.h"
#include "common/account/account_util.h"

namespace opera {

class AccountService;

/**
 * The interface for observers interested in AccountService events.
 */
class AccountObserver {
 public:
  virtual ~AccountObserver() {}

  /**
   * Called when the user becomes logged in.
   *
   * This happens when the AccountService has obtained the OAuth authorization
   * token and is ready to "sign" requests.
   *
   * @param account the AccountService issuing this call.
   * @param problem the auth problem that triggerred this OnLoggedIn() call.
   *                Note that the problem will have the source set to LOGIN
   *                in case of the initial callback. Note that the source will
   *                be set to RETRY in case a request is retried due to
   *                temporary circumstances.
   */
  virtual void OnLoggedIn(AccountService* service,
                          opera_sync::OperaAuthProblem problem) = 0;

  /**
   * Called after a failed log-in attempt.
   *
   * @param service the AccountService issuing this call.
   * @param error_code the failure error code. Currently one of:
   *        SyncAuthDataUpdater::OK
   *        SyncAuthDataUpdater::NETWORK_ERROR
   *        SyncAuthDataUpdater::HTTP_ERROR
   *        SyncAuthDataUpdater::AUTH_ERROR
   *        SyncAuthDataUpdater::PARSE_ERROR
   * @param auth_code the specific auth-related failure, only meaningful
   *        in case error_code equals to AUTH_ERROR. Note that the simple
   *        auth fetcher is unable to tell specific errors apart and will
   *        always return a generic error (compare AuthOperaComError).
   * @param message the error message received from the server.  Can be empty.
   * @param problem the auth problem that caused the call.
   */
  virtual void OnLoginError(AccountService* service,
                            account_util::AuthDataUpdaterError error_code,
                            account_util::AuthOperaComError auth_code,
                            const std::string& message,
                            opera_sync::OperaAuthProblem problem) = 0;

  /**
   * Called after LogOut() has been called for an account.
   * @param service the AccountService issuing this call.
   * @logout_reason the logout reason as passed by caller.
   */
  virtual void OnLoggedOut(AccountService* service,
                           account_util::LogoutReason logout_reason) = 0;

  /**
   * Called after AuthDataExpired() has been called for an account.
   * Note that in case AuthDataExpired() is called another time while
   * the token is being refreshed (i.e. the auth data has been cleared),
   * the call to AuthDataExpired() will be ignored and you will not get
   * this notification.
   * @param service the AccountService issuing this call.
   * @param problem the auth header verification problem that results in
   *        this call.
   */
  virtual void OnAuthDataExpired(AccountService* service,
                                 opera_sync::OperaAuthProblem problem) = 0;

  /**
   * Called after whenever LogIn() is trigerred, allows tracking the
   * reason for the login. Note that this call will precede any
   * calls that the LogIn() results in.
   * @param service the AccountService issuing this call.
   * @param problem the auth problem that caused the call.
   */
  virtual void OnLoginRequested(AccountService* service,
                                opera_sync::OperaAuthProblem problem) {}

  /**
  * Called after whenever LogOut() is trigerred. Note that this call will
  * precede any calls that the LogOut() results in.
  * @param service the AccountService issuing this call.
  * @logout_reason the logout reason as passed by caller.
  */
  virtual void OnLogoutRequested(AccountService* service,
                                 account_util::LogoutReason logout_reason) {}

  virtual void OnLoginRequestIgnored(AccountService* service,
                                     opera_sync::OperaAuthProblem problem) {}

  virtual void OnLoginRequestDeferred(AccountService* service,
                                      opera_sync::OperaAuthProblem problem,
                                      base::Time scheduled_time) {}
};

}  // namespace opera

#endif  // COMMON_ACCOUNT_ACCOUNT_OBSERVER_H_
