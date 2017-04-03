// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_LOGIN_DATA_STORE_H_
#define COMMON_SYNC_SYNC_LOGIN_DATA_STORE_H_

namespace opera {

struct SyncLoginData;

/**
 * A store of Sync log-in data.
 */
class SyncLoginDataStore {
 public:
  virtual ~SyncLoginDataStore() {}

  /**
   * Loads saved log-in data.
   *
   * The fields the could not be loaded are set to empty/default values.
   */
  virtual SyncLoginData LoadLoginData() const = 0;

  virtual void SaveLoginData(const SyncLoginData& login_data) = 0;

  /**
   * Clears the authorization token and session related data in the store.
   *
   * All other log-in data remains intact.
   */
  virtual void ClearSessionAndTokenData() = 0;
};

}  // namespace opera

#endif  // COMMON_SYNC_SYNC_LOGIN_DATA_STORE_H_
