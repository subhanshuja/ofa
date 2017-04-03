// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_AUTH_KEEPER_EVENT_RECORDER_H_
#define COMMON_SYNC_SYNC_AUTH_KEEPER_EVENT_RECORDER_H_

#include <vector>

#include "common/sync/sync_auth_keeper_observer.h"
namespace opera {
namespace sync {

class SyncAuthKeeperEventRecorder : public SyncAuthKeeperObserver {
 public:
  SyncAuthKeeperEventRecorder();
  ~SyncAuthKeeperEventRecorder() override;

  // SyncAuthKeeperObserver implementation
  void OnSyncAuthKeeperTokenStateChanged(
      SyncAuthKeeper* keeper,
      SyncAuthKeeper::TokenState state) override;
  void OnSyncAuthKeeperStatusChanged(SyncAuthKeeper* keeper) override;

  const std::vector<std::unique_ptr<base::DictionaryValue>>* events();

 protected:
  const unsigned kHistoricEventCountLimit = 50u;

  std::vector<std::unique_ptr<base::DictionaryValue>> historic_events_;
};

}  // namespace sync
}  // namespace opera

#endif  // COMMON_SYNC_SYNC_AUTH_KEEPER_EVENT_RECORDER_H_
