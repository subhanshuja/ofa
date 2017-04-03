// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_SESSION_OAUTH2_PERSISTENT_SESSION_MOCK_H_
#define COMMON_OAUTH2_SESSION_OAUTH2_PERSISTENT_SESSION_MOCK_H_

#include "testing/gmock/include/gmock/gmock.h"

#include "common/oauth2/session/persistent_session.h"

namespace opera {
namespace oauth2 {
class PersistentSessionMock : public PersistentSession {
 public:
  PersistentSessionMock();
  ~PersistentSessionMock() override;

  MOCK_METHOD1(Initialize, void(base::Closure));
  MOCK_METHOD0(LoadSession, void());
  MOCK_METHOD0(StoreSession, void());
  MOCK_METHOD0(ClearSession, void());
  MOCK_CONST_METHOD0(GetState, SessionState());
  MOCK_METHOD1(SetState, void(const SessionState&));
  MOCK_METHOD1(SetStartMethod, void(SessionStartMethod));
  MOCK_CONST_METHOD0(GetStartMethod, SessionStartMethod());
  MOCK_METHOD1(SetRefreshToken, void(const std::string&));
  MOCK_CONST_METHOD0(GetRefreshToken, std::string());
  MOCK_METHOD1(SetUsername, void(const std::string&));
  MOCK_CONST_METHOD0(GetUsername, std::string());
  MOCK_CONST_METHOD0(GetSessionId, std::string());
  MOCK_CONST_METHOD0(GetStartTime, base::Time());
  MOCK_METHOD1(SetUserId, void(const std::string&));
  MOCK_CONST_METHOD0(GetUserId, std::string());
  MOCK_CONST_METHOD0(IsInProgress, bool());
  MOCK_CONST_METHOD0(GetAuthError, OperaAuthError());
  MOCK_METHOD1(SetAuthError, void(OperaAuthError));
  MOCK_CONST_METHOD0(HasAuthError, bool());
  MOCK_CONST_METHOD0(IsLoggedIn, bool());
  MOCK_CONST_METHOD0(GetSessionIdForDiagnostics, std::string());
};
}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_SESSION_OAUTH2_PERSISTENT_SESSION_MOCK_H_
