// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/wand/wand_login.h"
#include "common/migration/wand/wand_reader.h"
#include "common/migration/tools/binary_reader.h"
#include "base/logging.h"

namespace opera {
namespace common {
namespace migration {

#if 0
const uint8_t WAND_OBJECTINFO_IS_PASSWORD = 0x01;
const uint8_t WAND_OBJECTINFO_IS_UNCHANGED = 0x02;
const uint8_t WAND_OBJECTINFO_IS_TEXTFIELD = 0x04;
const uint8_t WAND_OBJECTINFO_IS_GUESSED_USERNAME = 0x08;
#endif

bool WandLogin::Parse(WandReader* reader,
                      int32_t wand_version) {
  if (wand_version >= 6) {
    reader->Read<int32_t>();  // Dummy
    reader->ReadEncryptedString16();  // Dummy
    reader->ReadEncryptedString16();  // Dummy
  }

  url_ = reader->ReadEncryptedString16();
  username_ = reader->ReadEncryptedString16();
  password_ = reader->ReadEncryptedString16(true);

  return !reader->IsFailed();
}

bool WandLogin::operator==(const WandLogin& rhs) const {
  return url_ == rhs.url_ &&
      username_ == rhs.username_ &&
      password_ == rhs.password_;
}

}  // namespace migration
}  // namespace common
}  // namespace opera
