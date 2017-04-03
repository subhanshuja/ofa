// Copyright (C) 2015 Opera Software ASA.  All rights reserved.

#ifndef NET_TURBO_TURBO_CONSTANTS_H_
#define NET_TURBO_TURBO_CONSTANTS_H_

namespace net {

// Publicly exposed values for configuring Turbo 2 image quality.
enum class TurboImageQuality : int {
  NO_IMAGES = 1,
  LOW = 2,
  MEDIUM = 3,
  HIGH = 4,
  LOSSLESS = 5,
};

}  // namespace net

#endif  // NET_TURBO_TURBO_CONSTANTS_H_
