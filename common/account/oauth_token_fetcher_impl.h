// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_ACCOUNT_OAUTH_TOKEN_FETCHER_IMPL_H_
#define COMMON_ACCOUNT_OAUTH_TOKEN_FETCHER_IMPL_H_

#include <memory>
#include <string>

#include "base/memory/ref_counted.h"
#include "net/url_request/url_fetcher_delegate.h"

#include "common/account/oauth_token_fetcher.h"

namespace net {
class URLRequestContextGetter;
}

namespace opera {

class OAuthTokenFetcherImpl : public OAuthTokenFetcher,
                              public net::URLFetcherDelegate {
 public:
  /**
   * @param auth_server_url the URL from which to fetch OAuth authorization
   *    tokens
   * @param client_key the identifier portion of the client credentials.  A
   *    deprecated OAuth term for this is "Consumer Key".  Each Opera product
   *    is statically assigned a client key.  Must outlive this object.
   * @param request_context_getter provides request context for the fetcher.
   *    Must outlive this object.
   */
  OAuthTokenFetcherImpl(const std::string& auth_server_url,
                        const std::string& client_key,
                        net::URLRequestContextGetter* request_context_getter);
  ~OAuthTokenFetcherImpl() override;

  /**
   * @name URLFetcherDelegate implementation
   * @{
   */
  void OnURLFetchComplete(const net::URLFetcher* source) override;
  /** @} */

 private:
  /**
   * Must not be called while a previous fetch is in progress.
   */
  void Start() override;

  void ParseServerResponse(const std::string& response);

  const std::string auth_server_url_;
  const std::string client_key_;
  net::URLRequestContextGetter* request_context_getter_;
  std::unique_ptr<net::URLFetcher> fetcher_;
};

}  // namespace opera

#endif  // COMMON_ACCOUNT_OAUTH_TOKEN_FETCHER_IMPL_H_
