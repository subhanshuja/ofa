// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/migration/wand/ssl_password_reader.h"

#include <fstream>

#include "base/logging.h"

#include "common/migration/tools/data_stream_reader.h"

const uint8_t TAG_SSL_USER_PASSWORD_RECORD = 0x04;
const uint8_t TAG_SSL_PASSWD_MAINBLOCK = 0x50;
const uint8_t TAG_SSL_PASSWD_MAINSALT = 0x51;

namespace opera {
namespace common {
namespace migration {

SslPasswordReader::SslPasswordReader(const base::FilePath& path_to_opcertdat)
    : path_to_opcertdat_(path_to_opcertdat) {}

SslPasswordReader::~SslPasswordReader() {
}

bool SslPasswordReader::Read() {
  std::ifstream input(path_to_opcertdat_.value().c_str(), std::ios_base::binary);
  if (!input.is_open() || input.fail())
    return false;
  DataStreamReader reader(&input, true);

  while (!reader.IsEof() && !reader.IsFailed()) {
    uint32_t tag = reader.ReadTag();
    // Look for the record with PASSWD_ info
    if (tag != TAG_SSL_USER_PASSWORD_RECORD) {
      reader.SkipRecord();
    } else {
      // Found it.
      if (ParseSslPasswdRecord(&reader)) {
        break;  // We're done here, no need to loop further
      } else {
        LOG(ERROR) << "Error while parsing SSL_USER_PASSWORD_RECORD";
        return false;
      }
    }
  }

  return !reader.IsFailed() &&
      !system_part_password_.empty() &&
      !system_part_password_salt_.empty();
}

bool SslPasswordReader::ParseSslPasswdRecord(DataStreamReader* reader) {
  size_t record_size = reader->ReadSize();
  size_t record_end_position = reader->GetCurrentStreamPosition() +
                               record_size;

  while (!reader->IsEof() &&
         !reader->IsFailed() &&
         reader->GetCurrentStreamPosition() < record_end_position) {
    uint32_t tag = reader->ReadTag();
    if (tag == TAG_SSL_PASSWD_MAINBLOCK) {
      system_part_password_ = reader->ReadVectorWithSize<uint8_t>();
    } else if (tag == TAG_SSL_PASSWD_MAINSALT) {
      system_part_password_salt_ = reader->ReadVectorWithSize<uint8_t>();
    } else {
      reader->SkipRecord();
    }
  }
  return !reader->IsFailed();
}

const std::vector<uint8_t>&
SslPasswordReader::GetSystemPartPassword() const {
  return system_part_password_;
}

const std::vector<uint8_t>&
SslPasswordReader::GetSystemPartPasswordSalt() const {
  return system_part_password_salt_;
}

}  // namespace migration
}  // namespace common
}  // namespace opera
