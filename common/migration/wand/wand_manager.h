// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_WAND_WAND_MANAGER_H_
#define COMMON_MIGRATION_WAND_WAND_MANAGER_H_
#include <istream>
#include <vector>

#include "common/migration/wand/wand_login.h"
#include "common/migration/wand/wand_profile.h"

namespace opera {
namespace common {
namespace migration {

/** The toplevel container of Wand data. */
struct WandManager {
  WandManager();
  WandManager(const WandManager&);
  ~WandManager();
  bool Parse(WandReader* reader,
             int32_t wand_version);
  bool operator==(const WandManager& rhs) const;

  // Profiles have Pages, which store login data for various forms
  WandProfile log_profile_;
  /* There's usually only one profile in profiles_, the "Personal profile", and
   * it's not very interesting. Look inside the log_profile_ for the pages. */
  std::vector<WandProfile> profiles_;
  size_t current_profile_;  ///< Index within profiles_.
  // Logins have login data for simple prompts, ex. HTTP Auths
  std::vector<WandLogin> logins_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_WAND_WAND_MANAGER_H_
