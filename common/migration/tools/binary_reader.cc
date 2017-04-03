// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include "common/migration/tools/binary_reader.h"

namespace opera {
namespace common {
namespace migration {

BinaryReader::BinaryReader(std::istream* input,
                           bool input_is_big_endian) :
    input_(*input),
    input_is_big_endian_(input_is_big_endian) {
}

BinaryReader::~BinaryReader() {
}

bool BinaryReader::IsFailed() const {
  return input_.fail();
}

bool BinaryReader::IsEof() const {
  return input_.eof() || input_.peek() == std::char_traits<char>::eof();
}

}  // namespace migration
}  // namespace common
}  // namespace opera
