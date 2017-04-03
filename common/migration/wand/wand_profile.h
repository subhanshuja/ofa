// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_MIGRATION_WAND_WAND_PROFILE_H_
#define COMMON_MIGRATION_WAND_WAND_PROFILE_H_

#include <vector>

#include "base/strings/string16.h"

#include "common/migration/wand/wand_page.h"

namespace opera {
namespace common {
namespace migration {

/** Container for WandPage objects. */
struct WandProfile {
  WandProfile();
  WandProfile(const WandProfile&);
  ~WandProfile();
  bool Parse(WandReader* reader, int32_t wand_version);
  bool operator==(const WandProfile& rhs) const;

  base::string16 name_;  ///< Name of the profile, usually irrelevant
  std::vector<WandPage> pages_;  ///< All pages belonging to this profile
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_WAND_WAND_PROFILE_H_
