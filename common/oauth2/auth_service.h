// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_AUTH_SERVICE_H_
#define COMMON_OAUTH2_AUTH_SERVICE_H_

#include <string>

#include "components/keyed_service/core/keyed_service.h"

#include "common/oauth2/util/util.h"

class GURL;

namespace opera {
namespace oauth2 {

class AuthToken;
class AuthServiceClient;
class SessionStateObserver;
class PersistentSession;

class AuthService : public KeyedService {
 public:
  ~AuthService() override {}

  // KeyedService implementation.
  void Shutdown() override = 0;

  virtual void StartSessionWithAuthToken(const std::string& username,
                                         const std::string& auth_token) = 0;

  virtual void EndSession(SessionEndReason reason) = 0;

  virtual void RequestAccessToken(AuthServiceClient* client,
                                  ScopeSet scopes) = 0;

  virtual void SignalAccessTokenProblem(
      AuthServiceClient* client,
      scoped_refptr<const AuthToken> token) = 0;

  virtual void RegisterClient(AuthServiceClient* client,
                              const std::string& client_name) = 0;
  virtual void UnregisterClient(AuthServiceClient* client) = 0;

  virtual void AddSessionStateObserver(SessionStateObserver* observer) = 0;
  virtual void RemoveSessionStateObserver(SessionStateObserver* observer) = 0;

  virtual const PersistentSession* GetSession() const = 0;
};
}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_AUTH_SERVICE_H_
