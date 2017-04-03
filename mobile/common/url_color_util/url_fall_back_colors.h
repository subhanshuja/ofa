// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_COLOR_UTIL_FALLBACK_COLORS_H_
#define MOBILE_COMMON_COLOR_UTIL_FALLBACK_COLORS_H_

#include <stdint.h>

namespace mobile {

/** A table of 32 colors. */
class URLFallBackColors {
 public:
  /** nr must be in the range [0,32) or 0 will be returned. */
  static uint32_t GetFallBackColorNumber(unsigned int nr);
};

}  // namespace mobile

#endif  // MOBILE_COMMON_COLOR_UTIL_FALLBACK_COLORS_H_
