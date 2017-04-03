// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_LOGIN_DATA_H_
#define COMMON_SYNC_SYNC_LOGIN_DATA_H_

#include <stdint.h>

#include <string>

#include "base/time/time.h"

#include "common/account/account_auth_data.h"

namespace opera {

/**
 * Gathers the log-in data of a Sync account.
 */
struct SyncLoginData {
  SyncLoginData();
  SyncLoginData(const SyncLoginData&);
  ~SyncLoginData();

  /**
   * @return @c true if authorization token data is available.
   */
  bool has_auth_data() const {
    return !auth_data.token.empty() && !auth_data.token_secret.empty();
  }

  /** The name to be displayed in the UI. Can be either username or email. */
  const std::string& display_name() const {
    return used_username_to_login ? user_name : user_email;
  }

  /**
   * @return true iff all fields, including auth_data, have the same values.
   */
  bool operator==(const SyncLoginData& other) const;

  /** The user name used to log in. Can be empty. */
  std::string user_name;

  /** The email associated with the account. Can be empty. */
  std::string user_email;

  /** Whether username or email was used to login. */
  bool used_username_to_login;

  /** The password used to log in. */
  std::string password;

  /**
   * The id of the current session where session is a time spanning
   * between logging in and out.
   */
  std::string session_id;

  /**
   * The user ID as returned by the auth server. Note that we can't get
   * it via the simple login endpoint so this will not always be present
   * (i.e. it will be empty on mobile).
   */
  std::string user_id;

  /**
   * Number of seconds timestamps sent to auth servers need to be skewed to
   * keep tokens valid. Used for devices that have a system clock that is
   * way off. Zero when not used/needed.
   */
  int64_t time_skew;

  base::Time session_start_time;
  AccountAuthData auth_data;
};

/**
 * Parses JSON-encoded log-in data into a SyncLoginData object.
 *
 * @return @c false on parse error
 */
bool JSONToSyncLoginData(const std::string& json_login_data,
                         SyncLoginData* login_data,
                         std::string* error);

}  // namespace opera

#endif  // COMMON_SYNC_SYNC_LOGIN_DATA_H_
