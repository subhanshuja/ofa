// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_SYNC_SYNC_CREDENTIALS_STORE_IMPL_H_
#define MOBILE_COMMON_SYNC_SYNC_CREDENTIALS_STORE_IMPL_H_

#include <string>

#include "base/compiler_specific.h"

class PrefService;

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace mobile {

class SyncCredentialsStore {
 public:
  SyncCredentialsStore(PrefService* prefs);
  virtual ~SyncCredentialsStore() {}

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Returns password for login name if known.
  // Returns empty string in case of error or unknown login name.
  virtual std::string LoadPassword(const std::string& login_name);
  // Returns login name if available, otherwise empty string
  virtual std::string LoadLoginName();
  // Returns user name if available, otherwise empty string
  virtual std::string LoadUserName();
  virtual void Store(const std::string& login_name,
                     const std::string& user_name,
                     const std::string& password);
  virtual void Clear();

 private:
  PrefService* const prefs_;
};

}  // namespace mobile

#endif  // MOBILE_COMMON_SYNC_SYNC_CREDENTIALS_STORE_IMPL_H_
