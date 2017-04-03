// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sync_login_data_store_impl.h"

#include <string>

#include "base/base64.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "components/os_crypt/os_crypt.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "common/sync/sync_login_data.h"

namespace opera {
namespace {

const char kPath[] = "path.to.data";

// The type of value that should be saved in prefs.
enum DataKeyType {
  STRING,             // plain string
  BOOLEAN,            // plain bool
  INTEGER_AS_STRING,  // int source, string output
};

struct KeysData {
  const char* key;
  DataKeyType type;
  bool encrypted;
};

const KeysData kDataKeys[] = {
    {"user_name", STRING, false},
    {"user_email", STRING, false},
    {"used_username_to_login", BOOLEAN, false},
    {"auth_token", STRING, true},
    {"auth_token_secret", STRING, true},
    {"session_id", STRING, false},
    {"time_skew", INTEGER_AS_STRING, false},
};

bool LoginDataEqual(const SyncLoginData& x,
                    const SyncLoginData& y,
                    bool with_session_id) {
  return x.user_name == y.user_name && x.user_email == y.user_email &&
         x.used_username_to_login == y.used_username_to_login &&
         x.auth_data.token == y.auth_data.token &&
         x.auth_data.token_secret == y.auth_data.token_secret &&
         x.time_skew == y.time_skew &&
         (!with_session_id || x.session_id == y.session_id);
}

class SyncLoginDataStoreImplTest : public testing::Test {
#ifdef OS_MACOSX
 public:
  SyncLoginDataStoreImplTest() {
    OSCrypt::UseMockKeychain(true);
  }
  ~SyncLoginDataStoreImplTest() override {
    OSCrypt::UseMockKeychain(false);
  }
#endif  // OS_MACOSX
};


TEST_F(SyncLoginDataStoreImplTest, EmptyStoreHasNoData) {
  TestingPrefServiceSimple prefs;
  prefs.registry()->RegisterDictionaryPref(kPath);

  SyncLoginDataStoreImpl store(&prefs, kPath);
  const SyncLoginData data = store.LoadLoginData();
  EXPECT_TRUE(data.user_name.empty());
  EXPECT_TRUE(data.user_email.empty());
  EXPECT_EQ(true, data.used_username_to_login);
  EXPECT_TRUE(data.auth_data.token.empty());
  EXPECT_TRUE(data.auth_data.token_secret.empty());
  EXPECT_FALSE(data.session_id.empty());
  EXPECT_EQ(0, data.time_skew);
}

TEST_F(SyncLoginDataStoreImplTest, PrefilledStoreHasData) {
  base::DictionaryValue stored_data;
  std::string plaintext = "blah";
  std::string ciphertext;
  std::string ciphertext_base64;
  ASSERT_TRUE(OSCrypt::EncryptString(plaintext, &ciphertext));
  base::Base64Encode(ciphertext, &ciphertext_base64);
  int64_t testnumber = 1337;
  std::string testnumberAsString = base::Int64ToString(testnumber);
  bool testbool = true;
  for (size_t i = 0; i < arraysize(kDataKeys); ++i) {
    switch (kDataKeys[i].type) {
      case STRING:
        // In case of encrypted data, we must set something that can be
        // decrypted back. For other doesn't matter, so we just set the encoded
        // cipher text also.
        stored_data.SetString(
            kDataKeys[i].key,
            kDataKeys[i].encrypted ? ciphertext_base64 : plaintext);
        break;
      case BOOLEAN:
        stored_data.SetBoolean(kDataKeys[i].key, testbool);
        break;
      case INTEGER_AS_STRING:
        stored_data.SetString(kDataKeys[i].key, testnumberAsString);
        break;
      default:
        NOTREACHED();
        break;
    }
  }

  TestingPrefServiceSimple prefs;
  prefs.registry()->RegisterDictionaryPref(kPath);
  prefs.Set(kPath, stored_data);

  SyncLoginDataStoreImpl store(&prefs, kPath);
  SyncLoginData data;
  std::string gen_uuid = data.session_id;
  EXPECT_FALSE(gen_uuid.empty());
  data = store.LoadLoginData();
  EXPECT_EQ(plaintext, data.user_name);
  EXPECT_EQ(plaintext, data.user_email);
  EXPECT_EQ(true, data.used_username_to_login);
  EXPECT_EQ(plaintext, data.auth_data.token);
  EXPECT_EQ(plaintext, data.auth_data.token_secret);
  EXPECT_EQ(plaintext, data.session_id);
  EXPECT_EQ(testnumber, data.time_skew);
}

TEST_F(SyncLoginDataStoreImplTest, SavingDataWritesToPrefs) {
  TestingPrefServiceSimple prefs;
  prefs.registry()->RegisterDictionaryPref(kPath);

  SyncLoginData data;
  // The value is created in the following way:
  // "test " + kDataKeys[user_name_index].key
  data.user_name = "test user_name";
  data.user_email = "test user_email";
  data.used_username_to_login = true;
  data.auth_data.token = "test auth_token";
  data.auth_data.token_secret = "test auth_token_secret";
  data.session_id = "test session_id";
  data.time_skew = 1337;

  SyncLoginDataStoreImpl store(&prefs, kPath);
  store.SaveLoginData(data);

  const base::DictionaryValue* stored_data = prefs.GetDictionary(kPath);
  ASSERT_NE(nullptr, stored_data);
  for (size_t i = 0; i < arraysize(kDataKeys); ++i) {
    switch (kDataKeys[i].type) {
      case STRING: {
        std::string dummy;
        EXPECT_TRUE(stored_data->GetStringWithoutPathExpansion(kDataKeys[i].key,
                                                               &dummy));
        EXPECT_FALSE(dummy.empty());
        if (!kDataKeys[i].encrypted)
          EXPECT_EQ(std::string("test ") + kDataKeys[i].key, dummy);
        break;
      }
      case BOOLEAN: {
        bool dummy_bool;
        EXPECT_TRUE(stored_data->GetBooleanWithoutPathExpansion(
            kDataKeys[i].key, &dummy_bool));
        EXPECT_EQ(data.used_username_to_login, dummy_bool);
        break;
      }
      case INTEGER_AS_STRING: {
        std::string dummy_string;
        int64_t dummy_int;
        EXPECT_TRUE(stored_data->GetStringWithoutPathExpansion(kDataKeys[i].key,
                                                               &dummy_string));
        base::StringToInt64(dummy_string, &dummy_int);
        EXPECT_EQ(data.time_skew, dummy_int);
        break;
      }
      default:
        NOTREACHED();
        break;
    }
  }
}

TEST_F(SyncLoginDataStoreImplTest, CanLoadSavedData) {
  TestingPrefServiceSimple prefs;
  prefs.registry()->RegisterDictionaryPref(kPath);

  SyncLoginData saved_data;
  saved_data.user_name = "user name";
  saved_data.user_email = "user email";
  saved_data.used_username_to_login = true;
  saved_data.auth_data.token = "auth token";
  saved_data.auth_data.token_secret = "auth token secret";
  saved_data.session_id = "session_id";
  saved_data.time_skew = 1337;

  SyncLoginDataStoreImpl saving_store(&prefs, kPath);
  saving_store.SaveLoginData(saved_data);

  SyncLoginDataStoreImpl loading_store(&prefs, kPath);
  const SyncLoginData loaded_data = loading_store.LoadLoginData();
  EXPECT_TRUE(LoginDataEqual(saved_data, loaded_data, true));
}

TEST_F(SyncLoginDataStoreImplTest, CanLoadPartialSavedData) {
  TestingPrefServiceSimple prefs;
  prefs.registry()->RegisterDictionaryPref(kPath);

  SyncLoginData saved_data;
  saved_data.user_name = "user name";

  SyncLoginDataStoreImpl saving_store(&prefs, kPath);
  saving_store.SaveLoginData(saved_data);

  SyncLoginDataStoreImpl loading_store(&prefs, kPath);
  const SyncLoginData loaded_data = loading_store.LoadLoginData();
  EXPECT_TRUE(LoginDataEqual(saved_data, loaded_data, false));
}

TEST_F(SyncLoginDataStoreImplTest, ClearingTokenDataWritesToPrefs) {
  TestingPrefServiceSimple prefs;
  prefs.registry()->RegisterDictionaryPref(kPath);

  SyncLoginData data;
  data.user_name = "user name";
  data.auth_data.token = "auth token";
  data.auth_data.token_secret = "auth token secret";

  SyncLoginDataStoreImpl store(&prefs, kPath);
  store.SaveLoginData(data);
  ASSERT_NE(nullptr, prefs.GetDictionary(kPath));
  ASSERT_FALSE(prefs.GetDictionary(kPath)->empty());

  store.ClearSessionAndTokenData();
  const base::DictionaryValue* stored_data = prefs.GetDictionary(kPath);
  ASSERT_NE(nullptr, stored_data);
  EXPECT_FALSE(stored_data->HasKey("auth_token"));
  EXPECT_FALSE(stored_data->HasKey("auth_token_secret"));
  EXPECT_FALSE(stored_data->HasKey("session_id"));

  SyncLoginDataStoreImpl loading_store(&prefs, kPath);
  const SyncLoginData loaded_data = loading_store.LoadLoginData();
  EXPECT_EQ(data.user_name, loaded_data.user_name);
  EXPECT_TRUE(loaded_data.auth_data.token.empty());
  EXPECT_TRUE(loaded_data.auth_data.token_secret.empty());
  EXPECT_FALSE(loaded_data.session_id.empty());
}

}  // namespace
}  // namespace opera
