// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/wand/wand_manager.h"

#include <stdint.h>

#include "common/migration/wand/wand_reader.h"
#include "common/migration/tools/binary_reader.h"
#include "base/logging.h"

namespace opera {
namespace common {
namespace migration {

WandManager::WandManager()
  : current_profile_(0) {
}

WandManager::WandManager(const WandManager&) = default;

WandManager::~WandManager() {
}

bool WandManager::Parse(WandReader* reader,
                        int32_t wand_version) {
  int32_t number_of_profiles = 0;
  if (wand_version >= 5) {
    reader->Read<int32_t>();  // Dummy
    current_profile_ = reader->Read<int32_t>();
    reader->Read<int32_t>();  // Dummy
    reader->Read<int32_t>();  // Dummy
    reader->Read<int32_t>();  // Dummy
    reader->Read<int32_t>();  // Dummy
    number_of_profiles = reader->Read<int32_t>();
  } else {
    // wand versions 2-4
    number_of_profiles = reader->Read<int32_t>();
    current_profile_ = reader->Read<int32_t>();
  }

  // Read profiles
  for (int32_t i = 0; i < number_of_profiles && !reader->IsEof(); ++i) {
    WandProfile profile;
    if (!profile.Parse(reader, wand_version)) {
      LOG(ERROR) << "Cannot parse profile " << i;
      profiles_.clear();
      return false;
    }
    profiles_.push_back(profile);
  }
  if (!log_profile_.Parse(reader, wand_version)) {
    LOG(ERROR) << "Cannot parse log profile";
    profiles_.clear();
    return false;
  }

  // Read logins
  int32_t number_of_logins = reader->Read<int32_t>();
  for (int32_t i = 0; i < number_of_logins && !reader->IsEof(); ++i) {
    WandLogin login;
    if (!login.Parse(reader, wand_version)) {
      LOG(ERROR) << "Cannot parse login " << i;
      logins_.clear();
      return false;
    }
    logins_.push_back(login);
  }

  return !reader->IsFailed();
}

bool WandManager::operator==(const WandManager& rhs) const {
  return
      rhs.current_profile_ == current_profile_ &&
      rhs.logins_ == logins_ &&
      rhs.log_profile_ == log_profile_ &&
      rhs.profiles_ == profiles_;
}

}  // namespace migration
}  // namespace common
}  // namespace opera
