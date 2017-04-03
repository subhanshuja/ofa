// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_ACCOUNT_MOCK_ACCOUNT_OBSERVER_H_
#define COMMON_ACCOUNT_MOCK_ACCOUNT_OBSERVER_H_

#include <string>

#include "testing/gmock/include/gmock/gmock.h"
#include "url/gurl.h"

#include "common/account/account_observer.h"
#include "common/account/account_service.h"

namespace opera {

class MockAccountObserver : public AccountObserver {
 public:
  MockAccountObserver();
  ~MockAccountObserver();

  MOCK_METHOD2(OnLoggedIn,
               void(AccountService* service,
                    opera_sync::OperaAuthProblem problem));
  MOCK_METHOD5(OnLoginError,
               void(AccountService* service,
                    account_util::AuthDataUpdaterError error_code,
                    account_util::AuthOperaComError auth_code,
                    const std::string& message,
                    opera_sync::OperaAuthProblem problem));
  MOCK_METHOD2(OnLoggedOut, void(AccountService* account,
                            account_util::LogoutReason reason));
  MOCK_METHOD2(OnAuthDataExpired, void(AccountService* account,
                    opera_sync::OperaAuthProblem problem));
  MOCK_METHOD2(OnLoginRequested,
      void(AccountService* account,
      opera_sync::OperaAuthProblem problem));
};
}  // namespace opera

#endif  // COMMON_ACCOUNT_MOCK_ACCOUNT_OBSERVER_H_
