// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_ACCOUNT_ACCOUNT_AUTH_DATA_FETCHER_MOCK_H_
#define COMMON_ACCOUNT_ACCOUNT_AUTH_DATA_FETCHER_MOCK_H_

#include "common/account/account_auth_data_fetcher.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace opera {

class AccountAuthDataFetcherMock : public AccountAuthDataFetcher {
public:
  AccountAuthDataFetcherMock();
  ~AccountAuthDataFetcherMock();
  MOCK_METHOD3(RequestAuthData,
               void(opera_sync::OperaAuthProblem,
                    const RequestAuthDataSuccessCallback&,
                    const RequestAuthDataFailureCallback&));
};

}  // namespace opera

#endif  // COMMON_ACCOUNT_ACCOUNT_AUTH_DATA_FETCHER_MOCK_H_
