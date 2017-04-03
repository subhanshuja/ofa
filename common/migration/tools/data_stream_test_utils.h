// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_TOOLS_DATA_STREAM_TEST_UTILS_H_
#define COMMON_MIGRATION_TOOLS_DATA_STREAM_TEST_UTILS_H_
#include <algorithm>
#include <iostream>

#include "common/migration/tools/data_stream_reader.h"

namespace opera {
namespace common {
namespace migration {
namespace ut {

/** Inserts stuff into iostreams, optionally converts endianness. */
class StreamInserter {
 public:
  explicit StreamInserter(bool reverse_endianness)
      : reverse_endianness_(reverse_endianness) {}

  template<typename T>
  void Insert(const T& obj, std::iostream* stream) {;
    char array[sizeof(obj)];
    memcpy(array, reinterpret_cast<const void*>(&obj), sizeof(obj));
    if (reverse_endianness_)
      std::reverse(array, array + sizeof(array));
    stream->write(array, sizeof(array));
  }

  void Insert(const DataStreamReader::Spec& spec, std::iostream* stream) {
    Insert(spec.file_version, stream);
    Insert(spec.app_version, stream);
    Insert(spec.tag_length, stream);
    Insert(spec.len_length, stream);
  }

 private:
  bool reverse_endianness_;
};

}  // namespace ut
}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_TOOLS_DATA_STREAM_TEST_UTILS_H_
