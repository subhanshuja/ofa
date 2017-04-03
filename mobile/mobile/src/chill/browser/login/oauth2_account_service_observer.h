// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2017 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#ifndef CHILL_BROWSER_LOGIN_OAUTH2_ACCOUNT_SERVICE_OBSERVER_H_
#define CHILL_BROWSER_LOGIN_OAUTH2_ACCOUNT_SERVICE_OBSERVER_H_

#include <string>

namespace opera {

class OAuth2AccountServiceObserver {
 public:
  virtual ~OAuth2AccountServiceObserver() {}

  // Called when the AccountService is being destroyed
  virtual void OnServiceDestroyed() = 0;

  // Called when the OAuth2 session ended
  virtual void OnLoggedOut() = 0;
};

}  // namespace opera

#endif  // CHILL_BROWSER_LOGIN_OAUTH2_ACCOUNT_SERVICE_OBSERVER_H_
