// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sync_login_data_store_impl.h"

#include <string>
#include <utility>

#include "base/base64.h"
#include "base/strings/string_number_conversions.h"
#include "components/os_crypt/os_crypt.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/syncable_prefs/pref_service_syncable.h"

#include "common/constants/pref_names.h"
#include "common/sync/sync_config.h"
#include "common/sync/sync_login_data.h"

namespace opera {

namespace {

const char kAuthToken[] = "auth_token";
const char kAuthTokenSecret[] = "auth_token_secret";
const char kUserEmail[] = "user_email";
const char kUserId[] = "user_id";
const char kUserName[] = "user_name";
const char kSessionId[] = "session_id";
const char kSessionStartTime[] = "session_start_time";
const char kTimeSkew[] = "time_skew";
const char kUsedUsernameToLogin[] = "used_username_to_login";

std::string Encrypt(const std::string& plaintext) {
  std::string ciphertext;
  if (!plaintext.empty() &&
      OSCrypt::EncryptString(plaintext, &ciphertext)) {
    std::string ciphertext_base64;
    base::Base64Encode(ciphertext, &ciphertext_base64);
    return ciphertext_base64;
  }

  return std::string();
}

std::string Decrypt(const std::string& ciphertext_base64) {
  std::string ciphertext;
  if (!ciphertext_base64.empty() &&
      base::Base64Decode(ciphertext_base64, &ciphertext)) {
    std::string plaintext;
    if (OSCrypt::DecryptString(ciphertext, &plaintext))
      return plaintext;
  }

  return std::string();
}

}  // namespace

SyncLoginDataStoreImpl::SyncLoginDataStoreImpl(PrefService* prefs,
                                               const char* path)
    : prefs_(prefs), path_(path) {
}

// static
void SyncLoginDataStoreImpl::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterDictionaryPref(prefs::kSyncLoginData);
}

SyncLoginData SyncLoginDataStoreImpl::LoadLoginData() const {
  SyncLoginData result;

  const base::DictionaryValue* stored_data = prefs_->GetDictionary(path_);
  if (stored_data == nullptr || stored_data->empty())
    return result;

  stored_data->GetStringWithoutPathExpansion(kUserName, &result.user_name);
  stored_data->GetStringWithoutPathExpansion(kUserEmail, &result.user_email);
  stored_data->GetBooleanWithoutPathExpansion(kUsedUsernameToLogin,
                                              &result.used_username_to_login);
  stored_data->GetStringWithoutPathExpansion(kAuthToken,
                                             &result.auth_data.token);
  stored_data->GetStringWithoutPathExpansion(kAuthTokenSecret,
                                             &result.auth_data.token_secret);
  stored_data->GetStringWithoutPathExpansion(kSessionId, &result.session_id);
  std::string tmp;
  stored_data->GetStringWithoutPathExpansion(kTimeSkew, &tmp);
  base::StringToInt64(tmp, &result.time_skew);

  if ((SyncConfig::UsesOAuth2() || SyncConfig::ShouldUseAuthTokenRecovery()) &&
      stored_data->GetStringWithoutPathExpansion(kSessionStartTime, &tmp)) {
    stored_data->GetStringWithoutPathExpansion(kUserId, &result.user_id);
    int64_t tmp_time;
    base::StringToInt64(tmp, &tmp_time);
    result.session_start_time = base::Time::FromInternalValue(tmp_time);
  } else {
    result.user_id = std::string();
    result.session_start_time = base::Time();
  }

  // Some fields are stored encrypted.
  result.auth_data.token = Decrypt(result.auth_data.token);
  result.auth_data.token_secret = Decrypt(result.auth_data.token_secret);

  return result;
}

void SyncLoginDataStoreImpl::SaveLoginData(const SyncLoginData& login_data) {
  // Some fields are stored encrypted.
  SyncLoginData encrypted_data = login_data;
  encrypted_data.auth_data.token = Encrypt(login_data.auth_data.token);
  encrypted_data.auth_data.token_secret =
      Encrypt(login_data.auth_data.token_secret);

  DictionaryPrefUpdate updater(prefs_, path_);
  updater.Get()->SetStringWithoutPathExpansion(kUserName,
                                               encrypted_data.user_name);
  updater.Get()->SetStringWithoutPathExpansion(kUserEmail,
                                               encrypted_data.user_email);
  updater.Get()->SetBooleanWithoutPathExpansion(
      kUsedUsernameToLogin, encrypted_data.used_username_to_login);
  updater.Get()->SetStringWithoutPathExpansion(kAuthToken,
                                               encrypted_data.auth_data.token);
  updater.Get()->SetStringWithoutPathExpansion(
      kAuthTokenSecret, encrypted_data.auth_data.token_secret);
  updater.Get()->SetStringWithoutPathExpansion(kSessionId,
                                               encrypted_data.session_id);
  updater.Get()->SetStringWithoutPathExpansion(
      kTimeSkew, base::Int64ToString(encrypted_data.time_skew));

  if (SyncConfig::ShouldUseAuthTokenRecovery()) {
    updater.Get()->SetStringWithoutPathExpansion(kUserId, login_data.user_id);
    updater.Get()->SetStringWithoutPathExpansion(
        kSessionStartTime,
        base::Int64ToString(login_data.session_start_time.ToInternalValue()));
  }

  prefs_->CommitPendingWrite();
}

void SyncLoginDataStoreImpl::ClearSessionAndTokenData() {
  DictionaryPrefUpdate updater(prefs_, path_);
  updater.Get()->RemoveWithoutPathExpansion(kAuthToken, nullptr);
  updater.Get()->RemoveWithoutPathExpansion(kAuthTokenSecret, nullptr);
  updater.Get()->RemoveWithoutPathExpansion(kSessionId, nullptr);
  prefs_->CommitPendingWrite();
}

}  // namespace opera
