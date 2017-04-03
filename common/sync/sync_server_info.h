// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_SERVER_INFO_H_
#define COMMON_SYNC_SYNC_SERVER_INFO_H_

#include <string>

#include "base/time/time.h"

namespace opera {

/**
 * What we know about the Sync server.
 */
struct SyncServerInfo {
  enum ServerStatus {
    SYNC_UNKNOWN,
    SYNC_UNKNOWN_ERROR,
    SYNC_OK,

#define SYNC_SERVER_ERROR(label, value) SYNC_ ## label = value,
#include "common/sync/sync_server_error_codes.inc"
#undef SYNC_SERVER_ERROR
  };

  static const base::TimeDelta kIntervalUnknown;

  SyncServerInfo();

  /**
   * How often the server wants us to ask about changes on the server.  Equal
   * to #kIntervalUnknown if not known.
   */
  base::TimeDelta poll_interval;

  /**
   * How long the server wants us to wait before sending our local changes to
   * the server.  Equal to #kIntervalUnknown if not known.
   */
  base::TimeDelta send_interval;

  /**
   * The last status of the server known to us.
   */
  ServerStatus server_status;

  /**
   * Error statuses may be accompanied by an error message received from the
   * server.
   */
  std::string server_message;
};

}  // namespace opera

#endif  // COMMON_SYNC_SYNC_SERVER_INFO_H_
