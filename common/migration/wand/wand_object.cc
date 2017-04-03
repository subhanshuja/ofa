// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/wand/wand_object.h"
#include "common/migration/wand/wand_reader.h"
#include "common/migration/tools/binary_reader.h"
#include "base/logging.h"

namespace opera {
namespace common {
namespace migration {

const uint8_t WAND_OBJECTINFO_IS_PASSWORD = 0x01;
const uint8_t WAND_OBJECTINFO_IS_UNCHANGED = 0x02;
const uint8_t WAND_OBJECTINFO_IS_TEXTFIELD = 0x04;
const uint8_t WAND_OBJECTINFO_IS_GUESSED_USERNAME = 0x08;

WandObject::WandObject()
  : is_guessed_username_(false),
    is_password_(false),
    is_textfield_for_sure_(false),
    is_changed_(false) {
}

WandObject::WandObject(const WandObject&) = default;

WandObject::~WandObject() {
}

bool WandObject::Parse(WandReader* reader) {
  uint8_t flag = reader->Read<uint8_t>();

  is_password_ = (flag & WAND_OBJECTINFO_IS_PASSWORD) != 0;
  is_changed_ = (flag & WAND_OBJECTINFO_IS_UNCHANGED) == 0;
  is_textfield_for_sure_ = (flag & WAND_OBJECTINFO_IS_TEXTFIELD) != 0;
  is_guessed_username_ = (flag & WAND_OBJECTINFO_IS_GUESSED_USERNAME) != 0;

  name_ = reader->ReadEncryptedString16();
  value_ = reader->ReadEncryptedString16();
  password_ = reader->ReadEncryptedString16(true);

  return !reader->IsFailed();
}


bool WandObject::operator==(const WandObject& rhs) const {
  return
      is_changed_ == rhs.is_changed_ &&
      is_guessed_username_ == rhs.is_guessed_username_ &&
      is_password_ == rhs.is_password_ &&
      is_textfield_for_sure_ == rhs.is_textfield_for_sure_ &&
      name_ == rhs.name_ &&
      password_ == rhs.password_ &&
      value_ == rhs.value_;
}

}  // namespace migration
}  // namespace common
}  // namespace opera
