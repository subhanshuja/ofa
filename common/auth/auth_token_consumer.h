// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2017 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#ifndef COMMON_AUTH_AUTH_TOKEN_CONSUMER_H_
#define COMMON_AUTH_AUTH_TOKEN_CONSUMER_H_

#include <memory>
#include <string>

#include "common/auth/auth_service_error.h"

namespace opera {

class AuthTokenConsumer {
 public:
  struct UserData {
    UserData(const std::string& username,
             const std::string& email,
             int user_id);

    std::string username;
    std::string email;
    int user_id;

    bool operator==(const UserData& other) const;
  };

  virtual ~AuthTokenConsumer() {}

  virtual void OnAuthSuccess(const std::string& auth_token,
                             std::unique_ptr<UserData> user_data) = 0;
  virtual void OnAuthError(AuthServiceError error,
                           const std::string& error_msg) = 0;
};

}  // namespace opera

#endif  // COMMON_AUTH_AUTH_TOKEN_CONSUMER_H_
