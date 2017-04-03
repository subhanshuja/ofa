// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_LOGIN_DATA_STORE_MOCK_H_
#define COMMON_SYNC_SYNC_LOGIN_DATA_STORE_MOCK_H_

#include "common/sync/sync_login_data.h"
#include "common/sync/sync_login_data_store.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace opera {

class SyncLoginDataStoreMock : public SyncLoginDataStore {
 public:
  SyncLoginDataStoreMock();
  ~SyncLoginDataStoreMock();
  MOCK_CONST_METHOD0(LoadLoginData, SyncLoginData());
  MOCK_METHOD1(SaveLoginData, void(const SyncLoginData&));
  MOCK_METHOD0(ClearSessionAndTokenData, void());
};

}  // namespace opera

#endif  // COMMON_SYNC_SYNC_LOGIN_DATA_STORE_MOCK_H_
