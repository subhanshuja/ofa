// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/network/request_throttler.h"

namespace opera {
namespace oauth2 {
namespace {
const net::BackoffEntry::Policy kDefaultBackoffPolicy = {
    // Number of initial errors (in sequence) to ignore before applying
    // exponential back-off rules.
    0,

    // Initial delay for exponential back-off in ms.
    1000,  // 1 second.

    // Factor by which the waiting time will be multiplied.
    2,

    // Fuzzing percentage. ex: 10% will spread requests randomly
    // between 90%-100% of the calculated time.
    0,  // 33%.

    // Maximum amount of time we are willing to delay our request in ms.
    5 * 60 * 1000,  // 5 minutes.

    // Time to keep an entry from being discarded even when it
    // has no significant state, -1 to never discard.
    -1,

    // Don't use initial delay unless the last request was an error.
    true,
};
}

RequestThrottler::RequestThrottler(base::TickClock* backoff_clock)
    : backoff_clock_(backoff_clock) {}

RequestThrottler::~RequestThrottler() {}

base::TimeDelta RequestThrottler::GetAndUpdateRequestDelay(
    std::string request_key) {
  if (request_delays_.count(request_key) == 0) {
    request_delays_[request_key].reset(
        new net::BackoffEntry(&kDefaultBackoffPolicy, backoff_clock_));
  }

  const auto& backoff_entry = request_delays_[request_key].get();
  DCHECK(backoff_entry);
  base::TimeDelta current_delay = backoff_entry->GetTimeUntilRelease();
  if (!backoff_entry->ShouldRejectRequest()) {
    VLOG(1) << "Not throttled";
    DCHECK(current_delay == base::TimeDelta());
    backoff_entry->Reset();
  } else {
    VLOG(1) << "Throttled";
    DCHECK(current_delay > base::TimeDelta());
  }
  backoff_entry->InformOfRequest(false);

  VLOG(2) << "Throttle delay for " << request_key
          << ": current = " << current_delay
          << "; next = " << backoff_entry->GetTimeUntilRelease();

  return current_delay;
}
}  // namespace oauth2
}  // namespace opera
