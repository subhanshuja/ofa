// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_AUTH_SERVICE_MOCK_H_
#define COMMON_OAUTH2_AUTH_SERVICE_MOCK_H_

#include <string>

#include "testing/gmock/include/gmock/gmock.h"

#include "common/oauth2/auth_service.h"
#include "common/oauth2/util/init_params.h"
#include "common/oauth2/util/util.h"

namespace opera {
namespace oauth2 {

class AuthServiceMock : public AuthService {
 public:
  AuthServiceMock();
  ~AuthServiceMock() override;

  MOCK_METHOD1(Initialize, void(InitParams*));
  MOCK_METHOD0(Shutdown, void());
  MOCK_METHOD2(StartSessionWithAuthToken,
               void(const std::string&, const std::string&));
  MOCK_METHOD1(EndSession, void(SessionEndReason));
  MOCK_METHOD2(RequestAccessToken,
               void(AuthServiceClient* client, ScopeSet scopes));
  MOCK_METHOD2(SignalAccessTokenProblem,
               void(AuthServiceClient* client,
                    scoped_refptr<const AuthToken> token));

  MOCK_METHOD2(RegisterClient,
               void(AuthServiceClient*, const std::string&));
  MOCK_METHOD1(UnregisterClient, void(AuthServiceClient*));

  MOCK_METHOD1(AddSessionStateObserver, void(SessionStateObserver*));
  MOCK_METHOD1(RemoveSessionStateObserver,
               void(SessionStateObserver*));

  MOCK_CONST_METHOD0(GetSession, PersistentSession*());
};
}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_AUTH_SERVICE_MOCK_H_
