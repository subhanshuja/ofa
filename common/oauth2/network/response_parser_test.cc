// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/network/response_parser.h"

#include <memory>

#include "base/values.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "common/oauth2/util/constants.h"
#include "common/oauth2/test/testing_constants.h"

namespace opera {
namespace oauth2 {

TEST(ResponseParserTest, Smoke) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  dict->SetString(kAccessToken, test::kMockTokenValue1);
  dict->SetString(kRefreshToken, test::kMockTokenValue2);
  dict->SetInteger(kExpiresIn, 42);

  ResponseParser parser;
  parser.Expect(kAccessToken, ResponseParser::STRING,
                ResponseParser::IS_REQUIRED);
  parser.Expect(kRefreshToken, ResponseParser::STRING,
                ResponseParser::IS_REQUIRED);
  parser.Expect(kExpiresIn, ResponseParser::INTEGER,
                ResponseParser::IS_REQUIRED);

  ASSERT_TRUE(parser.Parse(dict.get(), ResponseParser::PARSE_STRICT));
  EXPECT_TRUE(parser.HasString(kAccessToken));
  EXPECT_TRUE(parser.HasString(kRefreshToken));
  EXPECT_TRUE(parser.HasInteger(kExpiresIn));
  EXPECT_FALSE(parser.HasInteger(kAccessToken));
  EXPECT_FALSE(parser.HasInteger(kRefreshToken));
  EXPECT_FALSE(parser.HasString(kExpiresIn));
  EXPECT_EQ(test::kMockTokenValue1, parser.GetString(kAccessToken));
  EXPECT_EQ(test::kMockTokenValue2, parser.GetString(kRefreshToken));
  EXPECT_EQ(42, parser.GetInteger(kExpiresIn));
}

TEST(ResponseParserTest, OptionalPresent) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  dict->SetString(kAccessToken, test::kMockTokenValue1);
  dict->SetString(kRefreshToken, test::kMockTokenValue2);
  dict->SetInteger(kExpiresIn, 42);

  ResponseParser parser;
  parser.Expect(kAccessToken, ResponseParser::STRING,
                ResponseParser::IS_OPTIONAL);
  parser.Expect(kRefreshToken, ResponseParser::STRING,
                ResponseParser::IS_REQUIRED);
  parser.Expect(kExpiresIn, ResponseParser::INTEGER,
                ResponseParser::IS_REQUIRED);

  ASSERT_TRUE(parser.Parse(dict.get(), ResponseParser::PARSE_STRICT));
  EXPECT_TRUE(parser.HasString(kAccessToken));
  EXPECT_TRUE(parser.HasString(kRefreshToken));
  EXPECT_TRUE(parser.HasInteger(kExpiresIn));
  EXPECT_FALSE(parser.HasInteger(kAccessToken));
  EXPECT_FALSE(parser.HasInteger(kRefreshToken));
  EXPECT_FALSE(parser.HasString(kExpiresIn));
  EXPECT_EQ(test::kMockTokenValue1, parser.GetString(kAccessToken));
  EXPECT_EQ(test::kMockTokenValue2, parser.GetString(kRefreshToken));
  EXPECT_EQ(42, parser.GetInteger(kExpiresIn));
}

TEST(ResponseParserTest, OptionalMissing) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  dict->SetString(kRefreshToken, test::kMockTokenValue2);
  dict->SetInteger(kExpiresIn, 42);

  ResponseParser parser;
  parser.Expect(kAccessToken, ResponseParser::STRING,
                ResponseParser::IS_OPTIONAL);
  parser.Expect(kRefreshToken, ResponseParser::STRING,
                ResponseParser::IS_REQUIRED);
  parser.Expect(kExpiresIn, ResponseParser::INTEGER,
                ResponseParser::IS_REQUIRED);

  ASSERT_TRUE(parser.Parse(dict.get(), ResponseParser::PARSE_STRICT));
  EXPECT_FALSE(parser.HasString(kAccessToken));
  EXPECT_TRUE(parser.HasString(kRefreshToken));
  EXPECT_TRUE(parser.HasInteger(kExpiresIn));
  EXPECT_FALSE(parser.HasInteger(kAccessToken));
  EXPECT_FALSE(parser.HasInteger(kRefreshToken));
  EXPECT_FALSE(parser.HasString(kExpiresIn));
  EXPECT_EQ(test::kMockTokenValue2, parser.GetString(kRefreshToken));
  EXPECT_EQ(42, parser.GetInteger(kExpiresIn));
}

TEST(ResponseParserTest, RequiredMissing) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  dict->SetString(kRefreshToken, test::kMockTokenValue2);
  dict->SetInteger(kExpiresIn, 42);

  ResponseParser parser;
  parser.Expect(kAccessToken, ResponseParser::STRING,
                ResponseParser::IS_REQUIRED);
  parser.Expect(kRefreshToken, ResponseParser::STRING,
                ResponseParser::IS_REQUIRED);
  parser.Expect(kExpiresIn, ResponseParser::INTEGER,
                ResponseParser::IS_REQUIRED);

  ASSERT_FALSE(parser.Parse(dict.get(), ResponseParser::PARSE_STRICT));
  EXPECT_FALSE(parser.HasString(kAccessToken));
  EXPECT_FALSE(parser.HasString(kRefreshToken));
  EXPECT_FALSE(parser.HasInteger(kExpiresIn));
  EXPECT_FALSE(parser.HasInteger(kAccessToken));
  EXPECT_FALSE(parser.HasInteger(kRefreshToken));
  EXPECT_FALSE(parser.HasString(kExpiresIn));
}

TEST(ResponseParserTest, NotExpectedPresent) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  dict->SetString(kAccessToken, test::kMockTokenValue1);
  dict->SetString(kRefreshToken, test::kMockTokenValue2);
  dict->SetInteger(kExpiresIn, 42);

  ResponseParser parser;
  parser.Expect(kRefreshToken, ResponseParser::STRING,
                ResponseParser::IS_REQUIRED);
  parser.Expect(kExpiresIn, ResponseParser::INTEGER,
                ResponseParser::IS_REQUIRED);

  ASSERT_FALSE(parser.Parse(dict.get(), ResponseParser::PARSE_STRICT));
  EXPECT_FALSE(parser.HasString(kAccessToken));
  EXPECT_FALSE(parser.HasString(kRefreshToken));
  EXPECT_FALSE(parser.HasInteger(kExpiresIn));
  EXPECT_FALSE(parser.HasInteger(kAccessToken));
  EXPECT_FALSE(parser.HasInteger(kRefreshToken));
  EXPECT_FALSE(parser.HasString(kExpiresIn));
}

TEST(ResponseParserTest, FailForEmptyString) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  dict->SetString(kAccessToken, test::kMockTokenValue1);
  dict->SetString(kRefreshToken, std::string());
  dict->SetInteger(kExpiresIn, 42);

  ResponseParser parser;
  parser.Expect(kRefreshToken, ResponseParser::STRING,
                ResponseParser::IS_REQUIRED);
  parser.Expect(kExpiresIn, ResponseParser::INTEGER,
                ResponseParser::IS_REQUIRED);

  ASSERT_FALSE(parser.Parse(dict.get(), ResponseParser::PARSE_STRICT));
  EXPECT_FALSE(parser.HasString(kAccessToken));
  EXPECT_FALSE(parser.HasString(kRefreshToken));
  EXPECT_FALSE(parser.HasInteger(kExpiresIn));
  EXPECT_FALSE(parser.HasInteger(kAccessToken));
  EXPECT_FALSE(parser.HasInteger(kRefreshToken));
  EXPECT_FALSE(parser.HasString(kExpiresIn));
}

TEST(ResponseParserTest, FailForZeroInteger) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  dict->SetString(kAccessToken, test::kMockTokenValue1);
  dict->SetString(kRefreshToken, test::kMockTokenValue2);
  dict->SetInteger(kExpiresIn, 0);

  ResponseParser parser;
  parser.Expect(kRefreshToken, ResponseParser::STRING,
                ResponseParser::IS_REQUIRED);
  parser.Expect(kExpiresIn, ResponseParser::INTEGER,
                ResponseParser::IS_REQUIRED);

  ASSERT_FALSE(parser.Parse(dict.get(), ResponseParser::PARSE_STRICT));
  EXPECT_FALSE(parser.HasString(kAccessToken));
  EXPECT_FALSE(parser.HasString(kRefreshToken));
  EXPECT_FALSE(parser.HasInteger(kExpiresIn));
  EXPECT_FALSE(parser.HasInteger(kAccessToken));
  EXPECT_FALSE(parser.HasInteger(kRefreshToken));
  EXPECT_FALSE(parser.HasString(kExpiresIn));
}

TEST(ResponseParserTest, FailForNegativeInteger) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  dict->SetString(kAccessToken, test::kMockTokenValue1);
  dict->SetString(kRefreshToken, test::kMockTokenValue2);
  dict->SetInteger(kExpiresIn, -112);

  ResponseParser parser;
  parser.Expect(kRefreshToken, ResponseParser::STRING,
                ResponseParser::IS_REQUIRED);
  parser.Expect(kExpiresIn, ResponseParser::INTEGER,
                ResponseParser::IS_REQUIRED);

  ASSERT_FALSE(parser.Parse(dict.get(), ResponseParser::PARSE_STRICT));
  EXPECT_FALSE(parser.HasString(kAccessToken));
  EXPECT_FALSE(parser.HasString(kRefreshToken));
  EXPECT_FALSE(parser.HasInteger(kExpiresIn));
  EXPECT_FALSE(parser.HasInteger(kAccessToken));
  EXPECT_FALSE(parser.HasInteger(kRefreshToken));
  EXPECT_FALSE(parser.HasString(kExpiresIn));
}

TEST(ResponseParserTest, FailForBooleanWrongType) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  dict->SetString(kAccessToken, test::kMockTokenValue1);

  ResponseParser parser;
  parser.Expect(kAccessToken, ResponseParser::BOOLEAN,
                ResponseParser::IS_REQUIRED);

  ASSERT_FALSE(parser.Parse(dict.get(), ResponseParser::PARSE_STRICT));
  EXPECT_FALSE(parser.HasString(kAccessToken));
  EXPECT_FALSE(parser.HasBoolean(kAccessToken));
  EXPECT_FALSE(parser.HasInteger(kAccessToken));
}

TEST(ResponseParserTest, ParseSoftAllowsNotExpectedKeys) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  dict->SetString(kAccessToken, test::kMockTokenValue1);
  dict->SetString(kRefreshToken, test::kMockTokenValue2);
  dict->SetInteger(kExpiresIn, 2049);

  ResponseParser parser;
  parser.Expect(kRefreshToken, ResponseParser::STRING,
                ResponseParser::IS_REQUIRED);
  parser.Expect(kExpiresIn, ResponseParser::INTEGER,
                ResponseParser::IS_REQUIRED);

  ASSERT_TRUE(parser.Parse(dict.get(), ResponseParser::PARSE_SOFT));
  EXPECT_FALSE(parser.HasString(kAccessToken));
  EXPECT_TRUE(parser.HasString(kRefreshToken));
  EXPECT_TRUE(parser.HasInteger(kExpiresIn));
  EXPECT_FALSE(parser.HasInteger(kAccessToken));
  EXPECT_FALSE(parser.HasInteger(kRefreshToken));
  EXPECT_FALSE(parser.HasString(kExpiresIn));
  EXPECT_EQ(test::kMockTokenValue2, parser.GetString(kRefreshToken));
  EXPECT_EQ(2049, parser.GetInteger(kExpiresIn));
}

TEST(ResponseParserTest, RequiredCantBeNullParseStrict) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  dict->SetBoolean(kAccessToken, true);
  dict->Set(kRefreshToken, base::Value::CreateNullValue());
  dict->Set(kExpiresIn, base::Value::CreateNullValue());

  ResponseParser parser;
  parser.Expect(kAccessToken, ResponseParser::BOOLEAN,
                ResponseParser::IS_REQUIRED);
  parser.Expect(kRefreshToken, ResponseParser::STRING,
                ResponseParser::IS_REQUIRED);
  parser.Expect(kExpiresIn, ResponseParser::INTEGER,
                ResponseParser::IS_REQUIRED);

  ASSERT_FALSE(parser.Parse(dict.get(), ResponseParser::PARSE_STRICT));
  EXPECT_FALSE(parser.HasString(kAccessToken));
  EXPECT_FALSE(parser.HasInteger(kAccessToken));
  EXPECT_FALSE(parser.HasBoolean(kAccessToken));
  EXPECT_FALSE(parser.HasString(kRefreshToken));
  EXPECT_FALSE(parser.HasInteger(kRefreshToken));
  EXPECT_FALSE(parser.HasBoolean(kRefreshToken));
  EXPECT_FALSE(parser.HasString(kExpiresIn));
  EXPECT_FALSE(parser.HasInteger(kExpiresIn));
  EXPECT_FALSE(parser.HasBoolean(kExpiresIn));
}

TEST(ResponseParserTest, RequiredCantBeNullParseSoft) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  dict->SetBoolean(kAccessToken, true);
  dict->Set(kRefreshToken, base::Value::CreateNullValue());
  dict->Set(kExpiresIn, base::Value::CreateNullValue());

  ResponseParser parser;
  parser.Expect(kAccessToken, ResponseParser::BOOLEAN,
                ResponseParser::IS_REQUIRED);
  parser.Expect(kRefreshToken, ResponseParser::STRING,
                ResponseParser::IS_REQUIRED);
  parser.Expect(kExpiresIn, ResponseParser::INTEGER,
                ResponseParser::IS_REQUIRED);

  ASSERT_FALSE(parser.Parse(dict.get(), ResponseParser::PARSE_SOFT));
  EXPECT_FALSE(parser.HasString(kAccessToken));
  EXPECT_FALSE(parser.HasInteger(kAccessToken));
  EXPECT_FALSE(parser.HasBoolean(kAccessToken));
  EXPECT_FALSE(parser.HasString(kRefreshToken));
  EXPECT_FALSE(parser.HasInteger(kRefreshToken));
  EXPECT_FALSE(parser.HasBoolean(kRefreshToken));
  EXPECT_FALSE(parser.HasString(kExpiresIn));
  EXPECT_FALSE(parser.HasInteger(kExpiresIn));
  EXPECT_FALSE(parser.HasBoolean(kExpiresIn));
}

TEST(ResponseParserTest, OptionalCanBeNullParseStrict) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  dict->SetBoolean(kAccessToken, true);
  dict->Set(kRefreshToken, base::Value::CreateNullValue());
  dict->Set(kExpiresIn, base::Value::CreateNullValue());

  ResponseParser parser;
  parser.Expect(kAccessToken, ResponseParser::BOOLEAN,
                ResponseParser::IS_REQUIRED);
  parser.Expect(kRefreshToken, ResponseParser::STRING,
                ResponseParser::IS_OPTIONAL);
  parser.Expect(kExpiresIn, ResponseParser::INTEGER,
                ResponseParser::IS_OPTIONAL);

  ASSERT_TRUE(parser.Parse(dict.get(), ResponseParser::PARSE_STRICT));
  EXPECT_FALSE(parser.HasString(kAccessToken));
  EXPECT_FALSE(parser.HasInteger(kAccessToken));
  EXPECT_TRUE(parser.HasBoolean(kAccessToken));
  EXPECT_EQ(parser.GetBoolean(kAccessToken), true);
  EXPECT_FALSE(parser.HasString(kRefreshToken));
  EXPECT_FALSE(parser.HasInteger(kRefreshToken));
  EXPECT_FALSE(parser.HasBoolean(kRefreshToken));
  EXPECT_FALSE(parser.HasString(kExpiresIn));
  EXPECT_FALSE(parser.HasInteger(kExpiresIn));
  EXPECT_FALSE(parser.HasBoolean(kExpiresIn));
}

TEST(ResponseParserTest, OptionalCanBeNullParseSoft) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  dict->SetBoolean(kAccessToken, true);
  dict->Set(kRefreshToken, base::Value::CreateNullValue());
  dict->Set(kExpiresIn, base::Value::CreateNullValue());

  ResponseParser parser;
  parser.Expect(kAccessToken, ResponseParser::BOOLEAN,
                ResponseParser::IS_REQUIRED);
  parser.Expect(kRefreshToken, ResponseParser::STRING,
                ResponseParser::IS_OPTIONAL);
  parser.Expect(kExpiresIn, ResponseParser::INTEGER,
                ResponseParser::IS_OPTIONAL);

  ASSERT_TRUE(parser.Parse(dict.get(), ResponseParser::PARSE_SOFT));
  EXPECT_FALSE(parser.HasString(kAccessToken));
  EXPECT_FALSE(parser.HasInteger(kAccessToken));
  EXPECT_TRUE(parser.HasBoolean(kAccessToken));
  EXPECT_EQ(parser.GetBoolean(kAccessToken), true);
  EXPECT_FALSE(parser.HasString(kRefreshToken));
  EXPECT_FALSE(parser.HasInteger(kRefreshToken));
  EXPECT_FALSE(parser.HasBoolean(kRefreshToken));
  EXPECT_FALSE(parser.HasString(kExpiresIn));
  EXPECT_FALSE(parser.HasInteger(kExpiresIn));
  EXPECT_FALSE(parser.HasBoolean(kExpiresIn));
}

}  // namespace oauth2
}  // namespace opera
