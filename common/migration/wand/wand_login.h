// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_WAND_WAND_LOGIN_H_
#define COMMON_MIGRATION_WAND_WAND_LOGIN_H_

#include <stdint.h>

#include "base/strings/string16.h"

namespace opera {
namespace common {
namespace migration {

class WandReader;

/** Login data for a simple, unambiguous login prompt. Usually, if there's a
 * popup window, the login data ends up here. If it's a form embedded in a
 * webpage, see WandObject and WandPage.
 */
struct WandLogin {
  bool Parse(WandReader* reader, int32_t wand_version);
  bool operator==(const WandLogin& rhs) const;

  base::string16 url_;
  base::string16 username_;
  base::string16 password_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_WAND_WAND_LOGIN_H_
