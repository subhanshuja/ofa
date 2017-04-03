// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_COLOR_UTIL_HASHER_H_
#define MOBILE_COMMON_COLOR_UTIL_HASHER_H_

#include <stdint.h>

class GURL;

namespace mobile {

/** A wrapper of the MurmurHash3 32 bit result implementation. */
class URLHasher {
 public:
  static uint32_t Hash(const GURL& url);
};

}  // namespace mobile

#endif  // MOBILE_COMMON_COLOR_UTIL_HASHER_H_
