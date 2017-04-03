// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_ACCOUNT_ACCOUNT_AUTH_DATA_H_
#define COMMON_ACCOUNT_ACCOUNT_AUTH_DATA_H_

#include <string>

namespace opera {

struct AccountAuthData {
  bool operator==(const AccountAuthData& other) const {
    return token == other.token && token_secret == other.token_secret;
  }
  bool operator!=(const AccountAuthData& other) const {
    return !(*this == other);
  }

  /** The authorization token identifier. */
  std::string token;

  /** The authorization token shared-secret. */
  std::string token_secret;
};

}  // namespace opera

#endif  // COMMON_ACCOUNT_ACCOUNT_AUTH_DATA_H_
