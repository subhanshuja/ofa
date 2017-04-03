// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sync_login_data.h"

#include <string>

#include "base/strings/stringprintf.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace opera {
namespace {

TEST(SyncLoginDataTest, UserNameSpecified) {
  const char json_login_data[] =
      "{"
      "  \"auth_token\": \"value\","
      "  \"auth_token_secret\": \"secret value\","
      "  \"userName\": \"user name\""
      "}";

  SyncLoginData login_data;
  std::string error;

  ASSERT_TRUE(JSONToSyncLoginData(json_login_data, &login_data, &error));
  EXPECT_EQ("value", login_data.auth_data.token);
  EXPECT_EQ("secret value", login_data.auth_data.token_secret);
  EXPECT_EQ("user name", login_data.user_name);
}

TEST(SyncLoginDataTest, EmailSpecified) {
  const char json_login_data[] =
      "{"
      "  \"auth_token\": \"value\","
      "  \"auth_token_secret\": \"secret value\","
      "  \"userEmail\": \"a@b.com\""
      "}";

  SyncLoginData login_data;
  std::string error;

  ASSERT_TRUE(JSONToSyncLoginData(json_login_data, &login_data, &error));
  EXPECT_EQ("value", login_data.auth_data.token);
  EXPECT_EQ("secret value", login_data.auth_data.token_secret);
  EXPECT_EQ("a@b.com", login_data.user_email);
}

TEST(SyncLoginDataTest, NoUsedUsernameToLoginSpecified) {
  const char json_login_data[] =
      "{"
      "  \"auth_token\": \"value\","
      "  \"auth_token_secret\": \"secret value\","
      "  \"userName\": \"user name\""
      "}";

  SyncLoginData login_data;
  std::string error;

  ASSERT_TRUE(JSONToSyncLoginData(json_login_data, &login_data, &error));
  EXPECT_EQ("value", login_data.auth_data.token);
  EXPECT_EQ("secret value", login_data.auth_data.token_secret);
  EXPECT_EQ(true, login_data.used_username_to_login);
}

TEST(SyncLoginDataTest, UsedUsernameToLoginSpecified) {
  const char json_login_data[] =
      "{"
      "  \"auth_token\": \"value\","
      "  \"auth_token_secret\": \"secret value\","
      "  \"userName\": \"user name\","
      "  \"usedUsernameToLogin\": true"
      "}";

  SyncLoginData login_data;
  std::string error;

  ASSERT_TRUE(JSONToSyncLoginData(json_login_data, &login_data, &error));
  EXPECT_EQ("value", login_data.auth_data.token);
  EXPECT_EQ("secret value", login_data.auth_data.token_secret);
  EXPECT_EQ("user name", login_data.user_name);
  EXPECT_EQ(true, login_data.used_username_to_login);
}

TEST(SyncLoginDataTest, NoUserName) {
  const char json_login_data[] =
      "{"
      "  \"auth_token\": \"value\","
      "  \"auth_token_secret\": \"secret value\""
      "}";

  SyncLoginData login_data;
  std::string error;

  EXPECT_FALSE(JSONToSyncLoginData(json_login_data, &login_data, &error));
  EXPECT_FALSE(error.empty());
}

TEST(SyncLoginDataTest, NoDictionary) {
  const char json_login_data[] ="\"auth_token\": \"value\"";

  SyncLoginData login_data;
  std::string error;

  EXPECT_FALSE(JSONToSyncLoginData(json_login_data, &login_data, &error));
  EXPECT_FALSE(error.empty());
}

TEST(SyncLoginDataTest, NoJSON) {
  const char json_login_data[] ="{{";

  SyncLoginData login_data;
  std::string error;

  EXPECT_FALSE(JSONToSyncLoginData(json_login_data, &login_data, &error));
  EXPECT_FALSE(error.empty());
}

}  // namespace
}  // namespace opera
