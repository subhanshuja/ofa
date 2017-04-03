// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_NETWORK_REQUEST_THROTTLER_H_
#define COMMON_OAUTH2_NETWORK_REQUEST_THROTTLER_H_

#include <map>
#include <memory>
#include <string>

#include "base/time/time.h"
#include "net/base/backoff_entry.h"

namespace opera {
namespace oauth2 {

class RequestThrottler {
 public:
  explicit RequestThrottler(base::TickClock* backoff_clock);
  ~RequestThrottler();

  /**
   * Returns the current request delay for the given request key and also
   * updates the internal state to return an adapted delay with the next call
   * with same key.
   * The returned request delay depends on the time delta from the previous call
   * with the same key, in case the the current call is performed still within
   * the backoff period for the given key, the backoff period is extended and
   * the new value is returned. In case the current call is performed after the
   * backoff period has finished, the backoff period is reset and the default
   * backoff value is returned.
   */
  base::TimeDelta GetAndUpdateRequestDelay(std::string request_key);

 private:
  base::TickClock* backoff_clock_;
  std::map<std::string, std::unique_ptr<net::BackoffEntry>> request_delays_;
};
}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_NETWORK_REQUEST_THROTTLER_H_
