// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/wand/wand_page.h"
#include "common/migration/wand/wand_reader.h"
#include "common/migration/tools/binary_reader.h"
#include "base/logging.h"

namespace opera {
namespace common {
namespace migration {

int WandPage::WAND_FLAG_NEVER_ON_THIS_PAGE = 0x00000001;
// Remember passwords on this entire server
int WandPage::WAND_FLAG_ON_THIS_SERVER = 0x00000100;
// These two used ifdef WAND_ECOMMERCE_SUPPORT
int WandPage::WAND_FLAG_STORE_ECOMMERCE = 0x00000200;
int WandPage::WAND_FLAG_STORE_NOTHING_BUT_ECOMMERCE = 0x00000400;

WandPage::WandPage()
  : form_number_(0),
    flags_(0),
    offset_x_(-1),
    offset_y_(-1),
    document_x_(-1),
    document_y_(-1) {
}

WandPage::WandPage(const WandPage&) = default;

WandPage::~WandPage() {
}

bool WandPage::Parse(WandReader* reader,
                     int32_t wand_version) {
  if (wand_version >= 6) {
    // Ignoring sync data, importing without
    reader->Read<int32_t>();  // Dummy
    reader->ReadEncryptedString16();  // Dummy
    reader->ReadEncryptedString16();  // Dummy
  }

  url_ = reader->ReadEncryptedString16();
  submitname_ = reader->ReadEncryptedString16();
  if (wand_version >= 4) {
    topdoc_url_ = reader->ReadEncryptedString16();
    url_action_ = reader->ReadEncryptedString16();
  }

  form_number_ = reader->Read<int32_t>();
  offset_x_ = reader->Read<int32_t>();
  offset_y_ = reader->Read<int32_t>();
  document_x_ = reader->Read<int32_t>();
  document_y_ = reader->Read<int32_t>();
  flags_ = reader->Read<int32_t>();

  int32_t number_of_objects = reader->Read<int32_t>();
  for (int32_t i = 0; i < number_of_objects && !reader->IsEof(); ++i) {
    WandObject object;
    if (!object.Parse(reader)) {
      LOG(ERROR) << "Cannot parse WandObject " << i;
      objects_.clear();
      return false;
    }
    objects_.push_back(object);
  }

  return !reader->IsFailed();
}

bool WandPage::operator==(const WandPage& rhs) const {
  return
      rhs.document_x_ == document_x_ &&
      rhs.document_y_ == document_y_ &&
      rhs.flags_ == flags_ &&
      rhs.form_number_ == form_number_ &&
      rhs.objects_ == objects_ &&
      rhs.offset_x_ == offset_x_ &&
      rhs.offset_y_ == offset_y_ &&
      rhs.submitname_ == submitname_;
}

}  // namespace migration
}  // namespace common
}  // namespace opera
