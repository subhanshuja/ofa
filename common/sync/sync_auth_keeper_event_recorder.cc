// -*-Mode: c++; tab - width: 2; indent - tabs - mode: nil; c - basic - offset:
// 2 - *-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include <vector>

#include "common/sync/sync_auth_keeper_event_recorder.h"

namespace opera {
namespace sync {
SyncAuthKeeperEventRecorder::SyncAuthKeeperEventRecorder() {}

SyncAuthKeeperEventRecorder::~SyncAuthKeeperEventRecorder() {}

void SyncAuthKeeperEventRecorder::OnSyncAuthKeeperTokenStateChanged(
    SyncAuthKeeper* keeper,
    SyncAuthKeeper::TokenState state) {
  // Empty.
}

void SyncAuthKeeperEventRecorder::OnSyncAuthKeeperStatusChanged(
    SyncAuthKeeper* keeper) {
  DCHECK(keeper);
  historic_events_.push_back(keeper->status().AsDict());
  if (historic_events_.size() > kHistoricEventCountLimit) {
    historic_events_.erase(historic_events_.begin());
  }
}

const std::vector<std::unique_ptr<base::DictionaryValue>>*
SyncAuthKeeperEventRecorder::events() {
  return &historic_events_;
}
}  // namespace sync
}  // namespace opera
