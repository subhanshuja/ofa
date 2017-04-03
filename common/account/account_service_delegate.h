// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_ACCOUNT_ACCOUNT_SERVICE_DELEGATE_H_
#define COMMON_ACCOUNT_ACCOUNT_SERVICE_DELEGATE_H_

#include "common/account/account_auth_data_fetcher.h"

namespace opera {

/**
 * Handles authorization token data for an AccountService.
 *
 * It extends AccountAuthDataFetcher with abilities to invalidate or discard any
 * cached token data.
 */
class AccountServiceDelegate : public AccountAuthDataFetcher {
 public:
  /**
   * Informs that any current authorization token data known to
   * AccountServiceDelegate has expired. This typically happens when the token
   * is expired by the authorization server. The next call to
   * AccountAuthDataFetcher::RequestAuthData() must return fresh data.
   */
  virtual void InvalidateAuthData() = 0;

  /**
   * Informs about a user-requested log-out.
   */
  virtual void LoggedOut() = 0;
};

}  // namespace opera

#endif  // COMMON_ACCOUNT_ACCOUNT_SERVICE_DELEGATE_H_
