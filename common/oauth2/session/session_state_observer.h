// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_SESSION_SESSION_STATE_OBSERVER_H_
#define COMMON_OAUTH2_SESSION_SESSION_STATE_OBSERVER_H_

namespace opera {
namespace oauth2 {

class OAuth2StorableSession;

class SessionStateObserver {
 public:
  virtual ~SessionStateObserver() {}

  virtual void OnSessionStateChanged() = 0;
};
}  // namespace oauth2
}  // namespace opera

#endif  // COMMON_OAUTH2_SESSION_SESSION_STATE_OBSERVER_H_
