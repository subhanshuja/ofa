// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_NET_TURBO_ZERO_RATING_DELEGATE_H_
#define CHILL_BROWSER_NET_TURBO_ZERO_RATING_DELEGATE_H_

namespace turbo {
namespace zero_rating {

class Delegate {
 public:
  Delegate() {}
  virtual ~Delegate() {}

  virtual int MatchUrl(const char* url) = 0;

  static Delegate* get();
};

}  // namespace zero_rating
}  // namespace turbo

typedef void (*SetTurboZeroRatingDelegateSignature)(
    turbo::zero_rating::Delegate* delegate);

extern "C"
    __attribute__((visibility("default"))) void SetTurboZeroRatingDelegate(
        turbo::zero_rating::Delegate* delegate);

#endif  // CHILL_BROWSER_NET_TURBO_ZERO_RATING_DELEGATE_H_
