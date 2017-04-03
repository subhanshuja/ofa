// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_ACCOUNT_TIME_SKEW_RESOLVER_IMPL_H_
#define COMMON_ACCOUNT_TIME_SKEW_RESOLVER_IMPL_H_

#include "base/callback.h"
#include "base/threading/thread_checker.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

#include "common/account/time_skew_resolver.h"

namespace opera {

// Checks difference between local time on client and server side. If difference
// is smaller than 24h server should return TIME_OK result and 0 time skew.
// Otherwise we should get TIME_SKEW result and time skew that we should use to
// correct timestamps.
class TimeSkewResolverImpl : public TimeSkewResolver,
                             public net::URLFetcherDelegate,
                             public base::ThreadChecker {
 public:
  // Factory used to create net::URLFetcher instance
  typedef base::Callback<std::unique_ptr<net::URLFetcher>(
      int,
      const GURL&,
      net::URLFetcher::RequestType,
      net::URLFetcherDelegate*)>
      URLFetcherFactory;

  // Defines callback that will be called once time skew is resolved. We have to
  // make sure that TimeSkewResolver object still exists when callback will
  // be called.
  TimeSkewResolverImpl(URLFetcherFactory fetcher_factory,
                       net::URLRequestContextGetter* request_context,
                       const GURL& auth_server_url);
  ~TimeSkewResolverImpl() override;

  // Validates time on server and passes results to a callback.
  void RequestTimeSkew(ResultCallback callback) override;

  // net::URLFetcherDelegate implementation
  void OnURLFetchComplete(const net::URLFetcher* request) override;

  static std::unique_ptr<TimeSkewResolver> Create(
      net::URLRequestContextGetter* time_skew_request_context,
      const GURL& auth_server_url);

 private:
  URLFetcherFactory fetcher_factory_;
  net::URLRequestContextGetter* request_context_;
  std::unique_ptr<net::URLFetcher> ongoing_request_;
  ResultCallback callback_;
  GURL time_check_url_;
};

}  // namespace opera

#endif  // COMMON_ACCOUNT_TIME_SKEW_RESOLVER_IMPL_H_
