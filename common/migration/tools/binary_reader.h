// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_TOOLS_BINARY_READER_H_
#define COMMON_MIGRATION_TOOLS_BINARY_READER_H_
#include <stdint.h>
#include <istream>
#include <string>
#include <algorithm>
#include <vector>

#include "base/strings/string16.h"

namespace opera {
namespace common {
namespace migration {

/** A generic reader for getting stuff from binary files in a cultured matter.
 * This is a basic version, used for almost entirely unstructured binary files
 * (ex. wand.dat) or as a base class for more elaborate formats, ex.
 * DataStreamReader.
 */
class BinaryReader {
 public:
  BinaryReader(std::istream* input, bool input_is_big_endian = true);
  virtual ~BinaryReader();

  /** Reads raw data from stream and converts between big- and little-endian
   * if needed.
   *
   * Just takes sizeof(T) bytes from the stream, reverse them if it's big-endian,
   * and return them as a T object.
   *
   * sizeof(T) must be {1|2|4|8}, otherwise endianness conversion doesn't work.
   *
   * It's safe to call it when the input is in a failed state or on EOF. In
   * such cases, will return a default-constructed T.
   */
  template<typename T>
  T Read() {
    if (input_.fail())
      return T();
    char input_buffer[sizeof(T)];
    input_.read(input_buffer, sizeof(input_buffer));
    if (input_is_big_endian_)
      std::reverse(input_buffer, input_buffer + sizeof(input_buffer));
    return *reinterpret_cast<T*>(input_buffer);
  }

  /** Reads a vector of elements from the source.
   * Each element will have correct endianness.
   * @param size number of elements (not bytes) to read.
   */
  template<typename T>
  std::vector<T> ReadVector(size_t size) {
    std::vector<T> data;
    if (input_.fail())
      return data;
    for (size_t i = 0; i < size; ++i) {
      data.push_back(Read<T>());
    }
    return data;
  }

  /** Check if a failure has occured in the past. Since we can't throw
   * exceptions, it's a good idea to check this often. While it's safe to
   * call Read* methods on a failed reader, you'll get default-constructed
   * objects and no indication whether they were like this in the file, or
   * have been created as such because /some/ return value was needed.
   * @retval true if something bad has happened during reading and the stream
   * is compromised.
   * @retval false if everything's ok.
   */
  virtual bool IsFailed() const;

  /** @retval true if we've reached the end of stream OR if the next character
   * is an end of stream. You can loop with !IsEof() as a condition.
   */
  bool IsEof() const;

 protected:
  std::istream& input_;
  const bool input_is_big_endian_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_TOOLS_BINARY_READER_H_
