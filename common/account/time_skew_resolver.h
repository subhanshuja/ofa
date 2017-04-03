// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_ACCOUNT_TIME_SKEW_RESOLVER_H_
#define COMMON_ACCOUNT_TIME_SKEW_RESOLVER_H_

#include <string>
#include "base/callback.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace opera {

// Checks difference between local time on client and server side. If difference
// is smaller than 24h server should return TIME_OK result and 0 time skew.
// Otherwise we should get TIME_SKEW result and time skew that we should use to
// correct timestamps.
class TimeSkewResolver {
 public:
  // See AUTH-723 for details about the protocol and possible results.
  enum class QueryResult {
    TIME_OK = 200,    // no corrections needed
    TIME_SKEW = 401,  // corrections required

    // error on our side - fails auth request
    BAD_REQUEST_ERROR = 400,  // request error - see error message
    NO_RESPONSE_ERROR = 204,  // invalid configuration of URLFetcher - shouldn't
                              // happen

    // error on server side - try to ignore and move on
    INVALID_RESPONSE_ERROR = -1  // server sent invalid JSON
  };

  // On value.result == QueryResult.TIME_OK not use time_skew and leave
  // timestamp as is
  struct ResultValue {
    ResultValue(const QueryResult r, const int64_t ts, const std::string em)
        : result(r), time_skew(ts), error_message(em) {}
    const QueryResult result;
    const int64_t time_skew;
    const std::string error_message;
  };

  typedef base::Callback<void(const ResultValue&)> ResultCallback;

  virtual ~TimeSkewResolver() {}

  // Validates time on server and passes results to a callback.
  virtual void RequestTimeSkew(ResultCallback callback) = 0;
};

}  // namespace opera

#endif  // COMMON_ACCOUNT_TIME_SKEW_RESOLVER_H_
