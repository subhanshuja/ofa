// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/sync/sync_credentials_store.h"

#include <string>

#include "base/base64.h"
#include "components/os_crypt/os_crypt.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/syncable_prefs/pref_service_syncable.h"

namespace mobile {

namespace {

const char kSyncCredentials[] = "sync.credentials";

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


SyncCredentialsStore::SyncCredentialsStore(PrefService* prefs)
    : prefs_(prefs) {
}

// static
void SyncCredentialsStore::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterDictionaryPref(
      kSyncCredentials);
}

std::string SyncCredentialsStore::LoadPassword(const std::string& login_name) {
  const base::DictionaryValue* stored_data =
      prefs_->GetDictionary(kSyncCredentials);

  if (stored_data != nullptr && !stored_data->empty()) {
    std::string stored_login_name;
    stored_data->GetStringWithoutPathExpansion("user_name", &stored_login_name);
    if (stored_login_name == login_name) {
      std::string result;
      stored_data->GetStringWithoutPathExpansion("password", &result);
      return Decrypt(result);
    }
  }

  return std::string();
}

std::string SyncCredentialsStore::LoadLoginName() {
  const base::DictionaryValue* stored_data =
      prefs_->GetDictionary(kSyncCredentials);

  if (stored_data != nullptr && !stored_data->empty()) {
    std::string result;
    stored_data->GetStringWithoutPathExpansion("user_name", &result);
    return result;
  }

  return std::string();
}

std::string SyncCredentialsStore::LoadUserName() {
  const base::DictionaryValue* stored_data =
      prefs_->GetDictionary(kSyncCredentials);

  if (stored_data != nullptr && !stored_data->empty()) {
    std::string result;
    stored_data->GetStringWithoutPathExpansion("server_user_name", &result);
    return result;
  }

  return std::string();
}

void SyncCredentialsStore::Store(const std::string& login_name,
                                 const std::string& user_name,
                                 const std::string& password) {
  DictionaryPrefUpdate updater(prefs_, kSyncCredentials);
  updater.Get()->SetStringWithoutPathExpansion("user_name", login_name);
  updater.Get()->SetStringWithoutPathExpansion("server_user_name", user_name);
  std::string tmp = Encrypt(password);
  updater.Get()->SetStringWithoutPathExpansion("password", tmp);
  prefs_->CommitPendingWrite();
}

void SyncCredentialsStore::Clear() {
  DictionaryPrefUpdate updater(prefs_, kSyncCredentials);
  updater.Get()->RemoveWithoutPathExpansion("user_name", nullptr);
  updater.Get()->RemoveWithoutPathExpansion("server_user_name", nullptr);
  updater.Get()->RemoveWithoutPathExpansion("password", nullptr);
  prefs_->CommitPendingWrite();
}

}  // namespace mobile
