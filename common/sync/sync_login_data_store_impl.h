// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_LOGIN_DATA_STORE_IMPL_H_
#define COMMON_SYNC_SYNC_LOGIN_DATA_STORE_IMPL_H_

#include "base/compiler_specific.h"

#include "common/sync/sync_login_data_store.h"

class PrefService;

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace opera {

/**
 * The default implementation of SyncLoginDataStore.
 *
 * It is based on the Chromium preferences system, and is persistent if given
 * a persistent PrefService.
 *
 * It stores the authorization token and secret in an encrypted form using
 * Encryptor::EncryptString().
 */
class SyncLoginDataStoreImpl : public SyncLoginDataStore {
 public:
  /**
   * @param prefs the PrefService to use to store the log-in data
   * @param path the location of the log-in data dictionary within @p prefs
   */
  SyncLoginDataStoreImpl(PrefService* prefs, const char* path);

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  /**
   * @name SyncLoginDataStore implementation
   * @{
   */
  SyncLoginData LoadLoginData() const override;
  void SaveLoginData(const SyncLoginData& login_data) override;
  void ClearSessionAndTokenData() override;
  /** @} */

 private:
  PrefService* const prefs_;
  const char* const path_;
};

}  // namespace opera

#endif  // COMMON_SYNC_SYNC_LOGIN_DATA_STORE_IMPL_H_
