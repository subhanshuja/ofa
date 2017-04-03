// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_OBSERVER_H_
#define COMMON_SYNC_SYNC_OBSERVER_H_

namespace syncer {
class SyncService;
}

namespace opera {
/**
 * The interface for observers interested in Sync events.
 */
class SyncObserver {
 public:
  virtual ~SyncObserver() {}

  /**
   * Called when the status of the given sync service changes.
   *
   * To get the current status call SyncService::opera_sync_status().
   */
  virtual void OnSyncStatusChanged(syncer::SyncService* sync_service) = 0;
};
}  // namespace opera

#endif  // COMMON_SYNC_SYNC_OBSERVER_H_
