// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_ACCOUNT_ACCOUNT_AUTH_DATA_FETCHER_H_
#define COMMON_ACCOUNT_ACCOUNT_AUTH_DATA_FETCHER_H_

#include <string>

#include "base/callback.h"

#include "common/account/account_util.h"

#include "components/sync/util/opera_auth_problem.h"

namespace opera {

struct AccountAuthData;

/**
 * A source of authorization token data for AccountService.
 */
class AccountAuthDataFetcher {
 public:
  typedef base::Callback<void(const AccountAuthData& auth_data,
                              opera_sync::OperaAuthProblem problem)>
      RequestAuthDataSuccessCallback;
  typedef base::Callback<void(account_util::AuthDataUpdaterError error_code,
                              account_util::AuthOperaComError auth_code,
                              const std::string& message,
                              opera_sync::OperaAuthProblem problem)>
      RequestAuthDataFailureCallback;

  virtual ~AccountAuthDataFetcher() {}

  /**
   * Requests authorization token data.  The result callbacks can be run
   * asynchronously, but they have to run on the thread that called this
   * function.
   *
   * @param success_callback to be called to return the data requested
   * @param failure_callback to be called on failure to obtain the data
   *    requested
   */
  virtual void RequestAuthData(
      opera_sync::OperaAuthProblem problem,
      const RequestAuthDataSuccessCallback& success_callback,
      const RequestAuthDataFailureCallback& failure_callback) = 0;

  opera_sync::OperaAuthProblem auth_problem_;
};

}  // namespace opera

#endif  // COMMON_ACCOUNT_ACCOUNT_AUTH_DATA_FETCHER_H_
