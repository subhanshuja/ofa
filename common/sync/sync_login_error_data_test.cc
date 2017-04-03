// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sync_login_error_data.h"

#include <string>

#include "base/strings/string_number_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace opera {
namespace {

TEST(SyncLoginErrorDataTest, RecognizedJSON) {
  const char json_data[] =
      "{"
      "  \"err_code\": 99,"
      "  \"err_msg\": \"That's why.\""
      "}";

  std::string parse_error;
  SyncLoginErrorData data;

  ASSERT_TRUE(JSONToSyncLoginErrorData(json_data, &data, &parse_error));
  EXPECT_EQ(99, data.code);
  EXPECT_EQ("That's why.", data.message);
}

TEST(SyncLoginErrorDataTest, UnrecognizedJSON) {
  const char* json_data[] = {
    "{"
    "  \"err_code\": 99a,"
    "  \"err_msg\": \"err_code not a number.\""
    "}",
    "{"
    "  \"err_code\": \"99\","
    "  \"err_msg\": \"err_code not a number.\""
    "}",
    "{"
    "  \"err\": 9,"
    "  \"err_msg\": \"err_code not here.\""
    "}",
    "{"
    "  \"err_code\": 1"
    "}",
  };

  for (size_t i = 0; i < arraysize(json_data); ++i) {
    SCOPED_TRACE(base::SizeTToString(i));

    std::string parse_error;
    SyncLoginErrorData data;

    EXPECT_FALSE(JSONToSyncLoginErrorData(json_data[i], &data, &parse_error));
    EXPECT_FALSE(parse_error.empty());
  }
}

TEST(SyncLoginErrorDataTest, MalformedJSON) {
  const char json_data[] ="{\"\"\"}";

  std::string parse_error;
  SyncLoginErrorData data;

  EXPECT_FALSE(JSONToSyncLoginErrorData(json_data, &data, &parse_error));
  EXPECT_FALSE(parse_error.empty());
}

}  // namespace
}  // namespace opera
