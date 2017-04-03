// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sync_server_info.h"

namespace opera {

const base::TimeDelta SyncServerInfo::kIntervalUnknown;

SyncServerInfo::SyncServerInfo()
    : poll_interval(kIntervalUnknown),
      send_interval(kIntervalUnknown),
      server_status(SYNC_UNKNOWN) {
}

}  // namespace opera
