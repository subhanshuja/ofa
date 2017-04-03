// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_DELAY_PROVIDER_H_
#define COMMON_SYNC_SYNC_DELAY_PROVIDER_H_

#include "base/callback.h"
#include "base/time/time.h"

namespace opera {

/**
 * Used to do something after a delay.
 *
 * Abstracts a timer to facilitate unit tests of classes that need to delay
 * things.
 */
class SyncDelayProvider {
 public:
  virtual ~SyncDelayProvider() {}

  /**
   * Invokes a callback after some delay.
   *
   * @param delay the time to wait until invoking @p callback
   * @param callback the callback to invoke
   */
  virtual void InvokeAfter(base::TimeDelta delay,
                           const base::Closure& callback) = 0;

  /**
    * Returns true if the callback is already scheduled i.e.
    * InvokeAfter() has been called and neither Cancel() has been
    * nor the callback has been called.
    */
  virtual bool IsScheduled() = 0;

  /**
   * Cancels a previously requested delayed callback invocation.  Does
   * nothing if there is no callback waiting to be invoked.
   */
  virtual void Cancel() = 0;
};

}  // namespace opera

#endif  // COMMON_SYNC_SYNC_DELAY_PROVIDER_H_
