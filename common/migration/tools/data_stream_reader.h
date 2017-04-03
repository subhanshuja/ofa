// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#ifndef COMMON_MIGRATION_TOOLS_DATA_STREAM_READER_H_
#define COMMON_MIGRATION_TOOLS_DATA_STREAM_READER_H_
#include <stdint.h>
#include <algorithm>
#include <istream>
#include <string>
#include <vector>

#include "common/migration/tools/binary_reader.h"

namespace opera {
namespace common {
namespace migration {

/** Reads a DataStream - a widely used binary format from Presto.
 *
 * DataStreams are generic data containers that have several configuration
 * options and may store arbitrary data. The entity that does the reading must
 * be aware of the stream's semantics - after all, it's just a stream of bytes.
 *
 * DataStreamReader is a single-run reader, it treats the input stream as
 * forward-only.
 *
 * @warning Nothing is stopping you from reading what should be a string
 * into an integer - this is binary data and unless you have solid specs or
 * reverse engineering chops you might read garbage every now and then. This
 * is unavoidable due to how DataStream is composed.
 *
 * Typical usage example:
 * @code
 *
 * DataStreamReader reader(input, true);
 *
 * // Read the first tag. Note tht tag length is specified in the binary header
 * // that starts the input stream.
 * uint32_t tag = reader.ReadTag();
 *
 * // Semantics of a tag is not known to the DataStreamReader, user must decide
 * // what to do.
 * switch(tag) {
 * case 0x01:
 *   // According to specs, tag 0x01 signifies that what follows is an int16_t
 *   int16_t parsedInt = reader.ReadContentWithSize<int16_t>();
 *  ...
 * case 0x02:
 *   // Tag 0x02 means that there should be a string here
 *   std::string parsedString = reader.ReadString();
 * }
 *
 * if (reader.IsFailed())
 *   return;  // Reader won't throw, check IsFailed() often
 *
 * // Read next tag...
 * @endcode
 *
 */
class DataStreamReader : public BinaryReader {
 public:
  /** Creates a DataStreamReader that uses @a input as the stream. Nobody
   * else should modify the input stream while the DataStreamReader is using it.
   * The c-tor will attempt to read a Spec from the stream. If the values seem
   * bogus, the badbit will be set and IsFailed() will return true.
   *
   * @param input input stream. May be a file.
   * @param bool input_is_big_endian set to true of the input stream should be
   * interpreted as big endian.
   */
  DataStreamReader(std::istream* input, bool input_is_big_endian = false);

  /** The Spec header is expected to lie in the beggining of the input stream.
   *
   * It contains file version, app version, length of the tag (usually 1 byte)
   * and length of size (usually 2 bytes, which puts an upper limit on a
   * record's size to 255).
   */
  struct Spec {
    uint32_t file_version;
    uint32_t app_version;
    uint16_t tag_length;
    uint16_t len_length;
  };

  /** @returns the Spec structure parsed from the input stream */
  const Spec& GetSpec() const;

  /** Reads a tag from the stream.
   *
   * Tag's length is specified within the stream's header.
   *  It's usually one byte.
   */
  uint32_t ReadTag();

  /** First reads a size of the string, then the string itself.
   * Effectively a wrapper for ReadVectorWithSize<char>(), as std::string is
   * a vector of chars (with some added semantics).
   *
   * It expects the following binary input
   * (ex. for len_length == 2, big endian):
   * @code
   * // This is an example
   * 00 03 63 6F 6D
   * @endcode
   * In this case, 00 03 is the length (3), 63 6F 6D is the string ("com").
   * Would return "com".
   * @returns a parsed string
   */
  std::string ReadString();

  /** Reads size, then a vector of as many elements of type T that fit into
   * this size.
   */
  template<typename T>
  std::vector<T> ReadVectorWithSize() {
    return ReadVector<T>(ReadSize()/sizeof(T));
  }

  /** Reads size from the stream.
   *
   * Size's length is defined in the spec header (len_length). It's usually 2,
   * ie. stored on two bytes. In such case, pops two bytes from the stream and
   * iterprets them as the size of the following blob.
   */
  size_t ReadSize();

  /** Reads size and then content.
   * This is the usual method of storing elements in a DataStream. Example,
   * storing a time_t integer (len_length == 2, big endian):
   * @code
   * 00 08 00 00 00 00 50 8A 94 47
   * @endcode
   * 00 08 is the size part ("8 bytes follow"), then 00 00 00 00 50 8A 94 47
   * is the content part (0x50A89447).
   *
   * If the size read from stream doesn't match sizeof(T), returns a
   * default-constructed T and will now return true when IsFailed() is called.
   */
  template<typename T>
  T ReadContentWithSize() {
    if (sizeof(T) != ReadSize()) {
      input_.setstate(std::ios_base::badbit);
      return T();  // Error, I'd love to throw an exception here...
    }
    return Read<T>();
  }

  /** Skips as many bytes as will be read by ReadSize().
   *
   * The usual use case is:
   * @code
   * uint32_t tag = reader.ReadTag();
   * // This tag is not interesting, we want to check the next one.
   * // Size comes after the tag, so we are 'standing on it' right now.
   * reader.SkipRecord();
   * tag = reader.ReadTag();  // Tag of the next record
   * @endcode
   */
  void SkipRecord();

  /** Returns an integer describing the current stream position in number of
   * bytes.
   *
   * Useful when the stream's semantic is "Read this record for the next 0x3F
   * bytes, then there's another record".
   */
  uint32_t GetCurrentStreamPosition() const;

 private:
  Spec spec_;
};

}  // namespace migration
}  // namespace common
}  // namespace opera

#endif  // COMMON_MIGRATION_TOOLS_DATA_STREAM_READER_H_
