// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/net/turbo_zero_rating_delegate.h"

namespace turbo {
namespace zero_rating {

static Delegate* g_delegate;

Delegate* Delegate::get() { return g_delegate; }

}  // namespace zero_rating
}  // namespace turbo

void SetTurboZeroRatingDelegate(turbo::zero_rating::Delegate* delegate) {
  turbo::zero_rating::g_delegate = delegate;
}

