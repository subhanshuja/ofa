// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_ACCOUNT_MOCK_ACCOUNT_SERVICE_H_
#define COMMON_ACCOUNT_MOCK_ACCOUNT_SERVICE_H_

#include <string>

#include "testing/gmock/include/gmock/gmock.h"
#include "url/gurl.h"

#include "common/account/account_service.h"

namespace opera {

class MockAccountService : public AccountService {
 public:
  MockAccountService();
  virtual ~MockAccountService();

  MOCK_METHOD1(AddObserver, void(AccountObserver* observer));
  MOCK_METHOD1(RemoveObserver, void(AccountObserver* observer));
  MOCK_METHOD0(LogIn, void());
  MOCK_METHOD1(LogIn, void(opera_sync::OperaAuthProblem));
  MOCK_METHOD1(LogOut, void(opera::account_util::LogoutReason));
  MOCK_METHOD1(AuthDataExpired, void(opera_sync::OperaAuthProblem));
  MOCK_CONST_METHOD3(
      GetSignedAuthHeader, std::string(const GURL& request_base_url,
                                       HttpMethod http_method,
                                       const std::string& realm));
  MOCK_METHOD1(SetTimeSkew, void(int64_t time_skew));
  MOCK_CONST_METHOD0(HasAuthToken, bool());
  MOCK_CONST_METHOD0(next_token_request_time, base::Time());
  MOCK_METHOD1(SetRetryTimeout, void(const base::TimeDelta&));

  MOCK_METHOD2(OnAuthDataFetched,
               void(const AccountAuthData&, opera_sync::OperaAuthProblem));
  MOCK_METHOD4(OnAuthDataFetchFailed, void(
    account_util::AuthDataUpdaterError error_code,
    account_util::AuthOperaComError auth_code,
    const std::string& message,
    opera_sync::OperaAuthProblem problem));
  MOCK_METHOD1(DoRequestAuthData, void(opera_sync::OperaAuthProblem problem));
};

}  // namespace opera

#endif  // COMMON_ACCOUNT_MOCK_ACCOUNT_SERVICE_H_
