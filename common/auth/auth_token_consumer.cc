// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2017 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#include "common/auth/auth_token_consumer.h"

namespace opera {

AuthTokenConsumer::UserData::UserData(const std::string& username,
                                      const std::string& email,
                                      int user_id) {
  this->username = username;
  this->email = email;
  this->user_id = user_id;
}

bool AuthTokenConsumer::UserData::operator==(const UserData& other) const {
  return other.username == username && other.email == email &&
         other.user_id == user_id;
}

}  // namespace opera
