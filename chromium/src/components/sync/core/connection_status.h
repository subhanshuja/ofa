// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_CORE_CONNECTION_STATUS_H_
#define COMPONENTS_SYNC_CORE_CONNECTION_STATUS_H_

#if defined(OPERA_SYNC)
#include "components/sync/util/opera_auth_problem.h"
#endif  // OPERA_SYNC

namespace syncer {

#if !defined(OPERA_SYNC)
// Status of the sync connection to the server.
enum ConnectionStatus {
  CONNECTION_NOT_ATTEMPTED,
  CONNECTION_OK,
  CONNECTION_AUTH_ERROR,
  CONNECTION_SERVER_ERROR
};
#else
struct ConnectionStatus {
  enum StatusValue {
    CONNECTION_NOT_ATTEMPTED,
    CONNECTION_OK,
    CONNECTION_AUTH_ERROR,
    CONNECTION_SERVER_ERROR,
    // NOTE: This is Opera-specific!
    CONNECTION_UNAVAILABLE
  };

  ConnectionStatus() {}
  explicit ConnectionStatus(StatusValue v) { value = v; }

  StatusValue value;
#if defined(OPERA_SYNC)
  opera_sync::OperaAuthProblem auth_problem;
#endif  // OPERA_SYNC
};
#endif  // !OPERA_SYNC

}  // namespace syncer

#endif  // COMPONENTS_SYNC_CORE_CONNECTION_STATUS_H_
