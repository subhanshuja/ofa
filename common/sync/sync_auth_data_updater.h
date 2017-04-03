// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_AUTH_DATA_UPDATER_H_
#define COMMON_SYNC_SYNC_AUTH_DATA_UPDATER_H_

#include <memory>

#include "base/compiler_specific.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

#include "common/account/account_auth_data.h"
#include "common/account/account_auth_data_fetcher.h"

namespace net {
class URLFetcher;
class URLRequestContextGetter;
}

namespace opera {

/**
 * Exchanges expired authorization data for new data.
 */
class SyncAuthDataUpdater : public AccountAuthDataFetcher,
                            public net::URLFetcherDelegate {
 public:
  SyncAuthDataUpdater(net::URLRequestContextGetter* request_context_getter,
                      const AccountAuthData& old_auth_data);
  ~SyncAuthDataUpdater() override;

  /**
   * @name AccountAuthDataFetcher implementation
   * @{
   */
  void RequestAuthData(
      opera_sync::OperaAuthProblem problem,
      const RequestAuthDataSuccessCallback& success_callback,
      const RequestAuthDataFailureCallback& failure_callback) override;
  /** @} */

  /**
   * @name net::URLFetcherDelegate implementation
   * @{
   */
  void OnURLFetchComplete(const net::URLFetcher* source) override;
  /** @} */

 private:
  const GURL renewal_url_;
  net::URLRequestContextGetter* request_context_getter_;
  std::unique_ptr<net::URLFetcher> fetcher_;
  RequestAuthDataSuccessCallback success_callback_;
  RequestAuthDataFailureCallback failure_callback_;
  AccountAuthData old_auth_data_;
};

AccountAuthDataFetcher* CreateAuthDataUpdater(
    net::URLRequestContextGetter* request_context_getter,
    const AccountAuthData& old_auth_data);

}  // namespace opera

#endif  // COMMON_SYNC_SYNC_AUTH_DATA_UPDATER_H_
