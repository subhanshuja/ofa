// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_ACCOUT_MOCK_H_
#define COMMON_SYNC_SYNC_ACCOUT_MOCK_H_

#include "common/sync/sync_account.h"
#include "common/sync/sync_login_data.h"
#include "common/sync/sync_login_error_data.h"
#include "testing/gmock/include/gmock/gmock.h"

class SyncAccountMock : public opera::SyncAccount {
 public:
  SyncAccountMock();
  ~SyncAccountMock();
  MOCK_CONST_METHOD0(service, base::WeakPtr<opera::AccountService>());
  MOCK_METHOD1(SetLoginData, void(const opera::SyncLoginData&));
  MOCK_CONST_METHOD0(login_data, const opera::SyncLoginData&());
  MOCK_METHOD1(SetLoginErrorData, void(const opera::SyncLoginErrorData&));
  MOCK_CONST_METHOD0(login_error_data, const opera::SyncLoginErrorData&());
  MOCK_CONST_METHOD0(HasAuthData, bool());
  MOCK_CONST_METHOD0(HasValidAuthData, bool());
  MOCK_METHOD0(SyncEnabled, void());
};

#endif  // COMMON_SYNC_SYNC_ACCOUT_MOCK_H_
