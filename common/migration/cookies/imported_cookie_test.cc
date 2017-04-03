// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA
#include <cstdio>
#include <sstream>

#include "base/numerics/safe_conversions.h"
#include "common/migration/cookies/imported_cookie.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "common/migration/tools/data_stream_reader.h"

using ::testing::Test;

namespace opera {
namespace common {
namespace migration {

class ImportedCookieTest : public Test {
 protected:
  void InsertCorrectSpec(std::stringstream* input) const {
    DataStreamReader::Spec spec;
    spec.len_length = 1;  // Real code uses len_length = 2
    spec.tag_length = 1;
    spec.app_version = 33;
    spec.file_version= 55;
    input->write(reinterpret_cast<char*>(&spec), sizeof(spec));
  }
};


TEST_F(ImportedCookieTest, CookieWithoutName) {
  std::stringstream input;
  InsertCorrectSpec(&input);

  // Cookie has a value but doesn't have a name
  const char* cookie_binary = { "\x11\x07myvalue" };
  int8_t cookie_binary_size = base::checked_cast<int8_t>(strlen(cookie_binary));
  input.write(reinterpret_cast<char*>(&cookie_binary_size),
      sizeof(cookie_binary_size));
  input.write(cookie_binary, cookie_binary_size);

  DataStreamReader reader(&input);
  ASSERT_FALSE(reader.IsFailed());
  ImportedCookie cookie;
  EXPECT_FALSE(cookie.Parse(&reader));
}

TEST_F(ImportedCookieTest, BasicCookieWithNameAndValue) {
  std::stringstream input;
  InsertCorrectSpec(&input);

  const char* cookie_binary = { "\x11\x07myvalue\x10\x06myname" };
  int8_t cookie_binary_size = base::checked_cast<int8_t>(strlen(cookie_binary));
  input.write(reinterpret_cast<char*>(&cookie_binary_size),
      sizeof(cookie_binary_size));
  input.write(cookie_binary, cookie_binary_size);

  DataStreamReader reader(&input);
  ASSERT_FALSE(reader.IsFailed());
  ImportedCookie cookie;
  EXPECT_TRUE(cookie.Parse(&reader));
  EXPECT_EQ("myname=myvalue", cookie.CreateCookieLine());
}

TEST_F(ImportedCookieTest, CookieWithNameValueExpiry) {
  std::stringstream input;
  InsertCorrectSpec(&input);

  // Equivalent to Mon Oct 29 07:54:09 2012 GMT
  int64_t expiry_date = 1351497249;
  const char* cookie_binary = { "\x11"  // Name follows
                               "\x07"  // Name string length
                               "myvalue"  // Name string
                               "\x10"  // Value follows
                               "\x06"  // Value string length
                               "myname"  // Value string
                               "\x12"  // Expiry date follows
                               "\x08"  // Expiry date length (8 bytes time_t)
                               // Expiry date will be written separately
                             };
  int8_t cookie_binary_size = base::checked_cast<int8_t>(strlen(cookie_binary)
    + 8 /* 8 bytes for expiry date */);
  input.write(reinterpret_cast<char*>(&cookie_binary_size),
      sizeof(cookie_binary_size));
  input.write(cookie_binary, strlen(cookie_binary));
  input.write(reinterpret_cast<char*>(&expiry_date),
      sizeof(expiry_date));

  DataStreamReader reader(&input);
  ASSERT_FALSE(reader.IsFailed());
  ImportedCookie cookie;
  EXPECT_TRUE(cookie.Parse(&reader));
  EXPECT_EQ("myname=myvalue; Expires=Mon, 29-Oct-2012 07:54:09 GMT",
            cookie.CreateCookieLine());
}

TEST_F(ImportedCookieTest, CookieWithNameValueCommentPath) {
  std::stringstream input;
  InsertCorrectSpec(&input);

  const char* cookie_binary = { "\x11"  // Name follows
                               "\x07"  // Name string length
                               "myvalue"  // Name string
                               "\x10"  // Value follows
                               "\x06"  // Value string length
                               "myname"  // Value string
                               "\x14"  // Comment string follows
                               "\x08"  // Comment string length
                               "aComment"
                               "\x17"  // Path (RFC 2965) follows
                               "\x05"  // Path length
                               "path/"
                             };
  int8_t cookie_binary_size = base::checked_cast<int8_t>(strlen(cookie_binary));
  input.write(reinterpret_cast<char*>(&cookie_binary_size),
      sizeof(cookie_binary_size));
  input.write(cookie_binary, strlen(cookie_binary));

  DataStreamReader reader(&input);
  ASSERT_FALSE(reader.IsFailed());
  ImportedCookie cookie;
  EXPECT_TRUE(cookie.Parse(&reader));
  EXPECT_EQ("myname=myvalue; Comment=aComment; Path=path/",
            cookie.CreateCookieLine());
}

TEST_F(ImportedCookieTest, CookieWithNameValueVersionDomainSecure) {
  std::stringstream input;
  InsertCorrectSpec(&input);

  const char* cookie_binary = { "\x11"  // Name follows
                               "\x07"  // Name string length
                               "myvalue"
                               "\x10"  // Value follows
                               "\x06"  // Value string length
                               "myname"
                               "\x1A"  // Version follows
                               "\x01"  // Version length (int8_t, 1 byte)
                               "\x01"  // Version 1
                               "\x16"  // Domain (RFC 2965) follows
                               "\x0C"  // Domain length
                               "mydomain.com"
                               "\x99"  // Secure flag
                             };
  int8_t cookie_binary_size = base::checked_cast<int8_t>(strlen(cookie_binary));
  input.write(reinterpret_cast<char*>(&cookie_binary_size),
      sizeof(cookie_binary_size));
  input.write(cookie_binary, strlen(cookie_binary));

  DataStreamReader reader(&input);
  ASSERT_FALSE(reader.IsFailed());
  ImportedCookie cookie;
  EXPECT_TRUE(cookie.Parse(&reader));
  EXPECT_EQ("myname=myvalue; Domain=mydomain.com; Version=1; Secure",
            cookie.CreateCookieLine());
}

TEST_F(ImportedCookieTest, NotImportedTags) {
  std::stringstream input;
  InsertCorrectSpec(&input);

  const char* cookie_binary = { "\x11\x07myvalue"
                               "\x10\x06myname"
                               "\x13"  // TAG_COOKIE_LAST_USED time
                               "\x08"  // Last used time length (8 bytes)
                               "\x01\x01\x01\x01\x01\x01\x01\x01"  // Last used
                               "\x9B"  // TAG_COOKIE_ONLY_SERVER flag
                               "\x9C"  // TAG_COOKIE_PROTECTED flag
                               "\xA0"  // TAG_COOKIE_NOT_FOR_PREFIX flag
                               "\xA2"  // TAG_COOKIE_HAVE_PASSWORD flag
                               "\xA3"  // TAG_COOKIE_HAVE_AUTHENTICATION flag
                               "\xA4"  // TAG_COOKIE_ACCEPTED_AS_THIRDPARTY flag
                             };
  int8_t cookie_binary_size = base::checked_cast<int8_t>(strlen(cookie_binary));
  input.write(reinterpret_cast<char*>(&cookie_binary_size),
      sizeof(cookie_binary_size));
  input.write(cookie_binary, cookie_binary_size);

  DataStreamReader reader(&input);
  ASSERT_FALSE(reader.IsFailed());
  ImportedCookie cookie;
  EXPECT_TRUE(cookie.Parse(&reader));
  // Unsupported tags don't end up on the cookie line
  EXPECT_EQ("myname=myvalue", cookie.CreateCookieLine());
}

}  // namespace migration
}  // namespace common
}  // namespace opera
