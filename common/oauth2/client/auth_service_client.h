// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_CLIENT_AUTH_SERVICE_CLIENT_H_
#define COMMON_OAUTH2_CLIENT_AUTH_SERVICE_CLIENT_H_

#include <string>

#include "common/oauth2/util/util.h"

namespace opera {
namespace oauth2 {

class AuthService;

class AuthServiceClient {
 public:
  virtual void OnAccessTokenRequestCompleted(
      AuthService* service,
      RequestAccessTokenResponse response) = 0;
  virtual void OnAccessTokenRequestDenied(AuthService* service,
                                          ScopeSet requested_scopes) = 0;
};
}  // oauth2
}  // opera

#endif  // COMMON_OAUTH2_CLIENT_AUTH_SERVICE_CLIENT_H_
