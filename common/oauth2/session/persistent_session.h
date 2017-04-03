// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_SESSION_OAUTH2_PERSISTENT_SESSION_H_
#define COMMON_OAUTH2_SESSION_OAUTH2_PERSISTENT_SESSION_H_

#include <string>

#include "base/bind.h"
#include "base/time/time.h"

#include "common/oauth2/util/util.h"

namespace opera {
namespace oauth2 {

class PersistentSession {
 public:
  virtual ~PersistentSession() {}

  virtual void Initialize(base::Closure session_state_changed_callback) = 0;

  virtual void LoadSession() = 0;
  virtual void StoreSession() = 0;
  virtual void ClearSession() = 0;

  virtual SessionStartMethod GetStartMethod() const = 0;
  virtual void SetStartMethod(SessionStartMethod type) = 0;

  virtual SessionState GetState() const = 0;
  virtual void SetState(const SessionState& state) = 0;

  virtual std::string GetRefreshToken() const = 0;
  virtual void SetRefreshToken(const std::string& refresh_token) = 0;

  virtual std::string GetUsername() const = 0;
  virtual void SetUsername(const std::string& username) = 0;

  virtual std::string GetUserId() const = 0;
  virtual void SetUserId(const std::string& user_id) = 0;

  virtual std::string GetSessionId() const = 0;

  virtual base::Time GetStartTime() const = 0;

  virtual bool HasAuthError() const = 0;
  virtual bool IsLoggedIn() const = 0;
  virtual bool IsInProgress() const = 0;

  virtual std::string GetSessionIdForDiagnostics() const = 0;
};
}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_SESSION_OAUTH2_PERSISTENT_SESSION_H_
