// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include <stdint.h>
#include <cstdio>
#include <sstream>
#include <string>

#include "base/numerics/safe_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "common/migration/tools/data_stream_reader.h"
#include "common/migration/tools/data_stream_test_utils.h"

using opera::common::migration::ut::StreamInserter;
using ::testing::Test;

namespace opera {
namespace common {
namespace migration {

TEST(DataStreamReaderTest, GetSpec) {
  DataStreamReader::Spec spec;
  spec.len_length = 4;
  spec.tag_length = 2;
  spec.app_version = 33;
  spec.file_version= 55;

  std::stringstream input;
  input.write(reinterpret_cast<char*>(&spec), sizeof(spec));

  DataStreamReader reader(&input);
  ASSERT_FALSE(reader.IsFailed());
  const DataStreamReader::Spec& readSpec = reader.GetSpec();
  EXPECT_EQ(spec.app_version, readSpec.app_version);
  EXPECT_EQ(spec.file_version, readSpec.file_version);
  EXPECT_EQ(spec.tag_length, readSpec.tag_length);
  EXPECT_EQ(spec.len_length, readSpec.len_length);
}

TEST(DataStreamReaderTest, CorruptSpec_LenLength) {
  DataStreamReader::Spec spec;
  spec.len_length = 5;  // Shouldn't be more than 4
  spec.tag_length = 1;  // Correct

  std::stringstream input;
  input.write(reinterpret_cast<char*>(&spec), sizeof(spec));

  DataStreamReader reader(&input);
  EXPECT_TRUE(reader.IsFailed());
}

TEST(DataStreamReaderTest, CorruptSpec_Tagength) {
  DataStreamReader::Spec spec;
  spec.tag_length = 5;  // Shouldn't be more than 4
  spec.len_length = 2;  // Correct

  std::stringstream input;
  input.write(reinterpret_cast<char*>(&spec), sizeof(spec));

  DataStreamReader reader(&input);
  EXPECT_TRUE(reader.IsFailed());
}

TEST(DataStreamReaderTest, ReadTagOneByte) {
  DataStreamReader::Spec spec;
  spec.tag_length = 1;
  spec.len_length = 2;

  std::stringstream input;
  input.write(reinterpret_cast<char*>(&spec), sizeof(spec));
  uint8_t tags[] = { 14, 15, 16, 17 };
  input.write(reinterpret_cast<char*>(&tags), sizeof(tags));

  DataStreamReader reader(&input);
  EXPECT_FALSE(reader.IsFailed());
  EXPECT_FALSE(reader.IsEof());
  EXPECT_EQ(14u, reader.ReadTag());
  EXPECT_EQ(15u, reader.ReadTag());
  EXPECT_EQ(16u, reader.ReadTag());
  EXPECT_EQ(17u, reader.ReadTag());
  // There are no more tags:
  EXPECT_TRUE(reader.IsEof());
  reader.ReadTag();
  EXPECT_TRUE(reader.IsFailed());
}

TEST(DataStreamReaderTest, ReadTagTwoBytes) {
  DataStreamReader::Spec spec;
  spec.tag_length = 2;
  spec.len_length = 2;

  std::stringstream input;
  input.write(reinterpret_cast<char*>(&spec), sizeof(spec));
  uint16_t tags[] = { 14, 15, 16, 17 };
  input.write(reinterpret_cast<char*>(&tags), sizeof(tags));

  DataStreamReader reader(&input);
  EXPECT_FALSE(reader.IsFailed());
  EXPECT_FALSE(reader.IsEof());
  EXPECT_EQ(14u, reader.ReadTag());
  EXPECT_EQ(15u, reader.ReadTag());
  EXPECT_EQ(16u, reader.ReadTag());
  EXPECT_EQ(17u, reader.ReadTag());
  // There are no more tags:
  EXPECT_TRUE(reader.IsEof());
  reader.ReadTag();
  EXPECT_TRUE(reader.IsFailed());
}

TEST(DataStreamReaderTest, ReadSizeAndContent) {
  struct Content {
    int32_t a;
    int32_t b;
    int8_t c;
  };

  DataStreamReader::Spec spec;
  spec.tag_length = 1;
  spec.len_length = 2;

  std::stringstream input;
  input.write(reinterpret_cast<char*>(&spec), sizeof(spec));

  Content written_content = {5, 6, 7};
  uint16_t content_size = sizeof(written_content);
  input.write(reinterpret_cast<char*>(&content_size),
      sizeof(content_size));
  input.write(reinterpret_cast<char*>(&written_content.a),
      sizeof(written_content.a));
  input.write(reinterpret_cast<char*>(&written_content.b),
      sizeof(written_content.b));
  input.write(reinterpret_cast<char*>(&written_content.c),
      sizeof(written_content.c));

  DataStreamReader reader(&input);
  EXPECT_FALSE(reader.IsFailed());

  EXPECT_EQ(sizeof(written_content), reader.ReadSize());
  EXPECT_EQ(5, reader.Read<int32_t>());
  EXPECT_EQ(6, reader.Read<int32_t>());
  EXPECT_EQ(7, reader.Read<int8_t>());
  EXPECT_FALSE(reader.IsFailed());
  EXPECT_TRUE(reader.IsEof());
}

TEST(DataStreamReaderTest, ReadContentWithSize_InvalidSize) {
  DataStreamReader::Spec spec;
  spec.tag_length = 1;
  spec.len_length = 2;

  std::stringstream input;
  input.write(reinterpret_cast<char*>(&spec), sizeof(spec));

  uint64_t number = 42;
  uint16_t content_size = sizeof(number) - 2;  // Note, invalid size
  input.write(reinterpret_cast<char*>(&content_size), sizeof(content_size));
  input.write(reinterpret_cast<char*>(&number), sizeof(number));

  DataStreamReader reader(&input);
  EXPECT_FALSE(reader.IsFailed());

  reader.ReadContentWithSize<uint64_t>();
  // Reader is in failed state due to size mismatch
  EXPECT_TRUE(reader.IsFailed());
}

TEST(DataStreamReaderTest, ReadContentWithSize) {
  DataStreamReader::Spec spec;
  spec.tag_length = 1;
  spec.len_length = 2;

  std::stringstream input;
  input.write(reinterpret_cast<char*>(&spec), sizeof(spec));

  uint64_t number = 42;
  uint16_t content_size = sizeof(number);
  input.write(reinterpret_cast<char*>(&content_size), sizeof(content_size));
  input.write(reinterpret_cast<char*>(&number), sizeof(number));

  DataStreamReader reader(&input);
  EXPECT_FALSE(reader.IsFailed());

  EXPECT_EQ(number, reader.ReadContentWithSize<uint64_t>());
  EXPECT_FALSE(reader.IsFailed());
  EXPECT_TRUE(reader.IsEof());
}

TEST(DataStreamReaderTest, ReadString) {
  DataStreamReader::Spec spec;
  spec.tag_length = 1;
  spec.len_length = 2;

  std::stringstream input;
  input.write(reinterpret_cast<char*>(&spec), sizeof(spec));

  const char* string_input = "This is a string";
  uint16_t content_size = base::checked_cast<uint16_t>(strlen(string_input));
  input.write(reinterpret_cast<char*>(&content_size), sizeof(content_size));
  input.write(string_input, content_size);

  DataStreamReader reader(&input);
  EXPECT_FALSE(reader.IsFailed());

  std::string output = reader.ReadString();
  EXPECT_EQ(output, string_input);
  EXPECT_FALSE(reader.IsFailed());
}

TEST(DataStreamReaderTest, BigEndianContent) {
  DataStreamReader::Spec spec;
  spec.tag_length = 1;
  spec.len_length = 2;

  StreamInserter inserter(true /* reverse endianness */);
  std::stringstream input;
  inserter.Insert(spec, &input);

  uint64_t number = 42;
  uint16_t size = sizeof(number);
  inserter.Insert(size, &input);
  inserter.Insert(number, &input);

  DataStreamReader reader(&input, true /* big endian input */);
  EXPECT_FALSE(reader.IsFailed());

  EXPECT_EQ(number, reader.ReadContentWithSize<uint64_t>());
  EXPECT_FALSE(reader.IsFailed());
  EXPECT_TRUE(reader.IsEof());
}

}  // namespace migration
}  // namespace common
}  // namespace opera
