// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/tools/data_stream_reader.h"
#include <cmath>
#include <vector>
#include "base/logging.h"

namespace opera {
namespace common {
namespace migration {

DataStreamReader::DataStreamReader(std::istream* input,
                                   bool input_is_big_endian)
  : BinaryReader(input, input_is_big_endian) {
  spec_.file_version = Read<uint32_t>();
  spec_.app_version = Read<uint32_t>();
  spec_.tag_length = Read<uint16_t>();
  spec_.len_length = Read<uint16_t>();

  /* If the file becomes corrupt and we read in bogus lengths, we might run into
   * problems later if we try to read, say 1097235 bytes into a uint32_t tag.
   *
   * This is an error condition and noone should call ReadX if DataStreamReader
   * reports IsFailed(), but we're protecting ourselves from a buffer overrun in
   * such situation.
   */
  if (spec_.len_length > 4 || spec_.len_length == 0) {
    spec_.len_length = 0;
    input_.setstate(std::ios_base::badbit);
  }
  if (spec_.tag_length > 4 || spec_.len_length == 0) {
    spec_.tag_length = 0;
    input_.setstate(std::ios_base::badbit);
  }
  if (IsFailed()) {
    LOG(WARNING) << "Data stream had invalid specification, input corrupted?";
  }
}

const DataStreamReader::Spec& DataStreamReader::GetSpec() const {
  return spec_;
}

uint32_t DataStreamReader::ReadTag() {
  switch (spec_.tag_length) {
    case 1: return Read<uint8_t>();
    case 2: return Read<uint16_t>();
    case 4: return Read<uint32_t>();
    default: return 0;
  }
}

std::string DataStreamReader::ReadString() {
  std::vector<char> buffer(ReadVectorWithSize<char>());
  return std::string(buffer.begin(), buffer.end());
}

size_t DataStreamReader::ReadSize() {
  switch (spec_.len_length) {
    case 1: return Read<uint8_t>();
    case 2: return Read<uint16_t>();
    case 4: return Read<uint32_t>();
    default: return 0;
  }
}

uint32_t DataStreamReader::GetCurrentStreamPosition() const {
  return input_.tellg();
}

void DataStreamReader::SkipRecord() {
  size_t record_size = ReadSize();
  input_.seekg(record_size, std::ios_base::cur);
}

}  // namespace migration
}  // namespace common
}  // namespace opera
