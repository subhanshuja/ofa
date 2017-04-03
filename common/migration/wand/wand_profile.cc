// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/wand/wand_profile.h"
#include "common/migration/wand/wand_reader.h"
#include "common/migration/tools/binary_reader.h"
#include "base/logging.h"

namespace opera {
namespace common {
namespace migration {

WandProfile::WandProfile() {
}

WandProfile::WandProfile(const WandProfile&) = default;

WandProfile::~WandProfile() {
}

bool WandProfile::Parse(WandReader* reader,
                        int32_t wand_version) {
  name_ = reader->ReadEncryptedString16();

  // "type" byte, 0 = WAND_LOG_PROFILE, 1 = WAND_PERSONAL_PROFILE
  reader->Read<uint8_t>();

  int32_t number_of_pages = reader->Read<int32_t>();
  for (int32_t i = 0; i < number_of_pages && !reader->IsEof(); ++i) {
    WandPage page;
    if (!page.Parse(reader, wand_version)) {
      LOG(ERROR) << "Cannot parse WandPage " << i;
      pages_.clear();
      return false;
    }
    pages_.push_back(page);
  }

  return !reader->IsFailed();
}

bool WandProfile::operator==(const WandProfile& rhs) const {
  return name_ == rhs.name_ && pages_ == rhs.pages_;
}

}  // namespace migration
}  // namespace common
}  // namespace opera
