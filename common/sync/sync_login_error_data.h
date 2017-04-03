// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_LOGIN_ERROR_DATA_H_
#define COMMON_SYNC_SYNC_LOGIN_ERROR_DATA_H_

#include <string>

namespace opera {

/**
 * Gathers the log-in error information for a Sync account.
 */
struct SyncLoginErrorData {
  enum { SYNC_LOGIN_UNKNOWN_ERROR = -1 };

  SyncLoginErrorData();

  /** The error code. */
  int code;

  /** The error message as received from the Auth server. */
  std::string message;
};


/**
 * Parses JSON-encoded log-in error data into a SyncLoginErrorData object.
 *
 * @return @c false on parse error
 */
bool JSONToSyncLoginErrorData(const std::string& json_login_error_data,
                              SyncLoginErrorData* login_error_data,
                              std::string* parse_error);

}  // namespace opera

#endif  // COMMON_SYNC_SYNC_LOGIN_ERROR_DATA_H_
