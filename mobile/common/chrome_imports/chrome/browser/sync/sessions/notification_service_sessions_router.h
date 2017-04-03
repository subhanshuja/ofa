// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_SESSIONS_NOTIFICATION_SERVICE_SESSIONS_ROUTER_H_
#define CHROME_BROWSER_SYNC_SESSIONS_NOTIFICATION_SERVICE_SESSIONS_ROUTER_H_

#include "components/sync_sessions/sessions_sync_manager.h"

class Profile;

namespace sync_sessions {

class SyncedTabDelegate;

// A SessionsSyncManager::LocalEventRouter that drives session sync via
// the NotificationService.
class NotificationServiceSessionsRouter
    : public LocalSessionEventRouter {
 public:
  NotificationServiceSessionsRouter(
      Profile* profile,
      sync_sessions::SyncSessionsClient* sessions_client,
      const syncer::SyncableService::StartSyncFlare& flare);
  virtual ~NotificationServiceSessionsRouter();

  // SessionsSyncManager::LocalEventRouter implementation.
  void StartRoutingTo(LocalSessionEventHandler* handler) override;
  void Stop() override;

  virtual void SignalTabChange(SyncedTabDelegate* tab);

 private:
  Profile* profile_;
  sync_sessions::SyncSessionsClient* const sessions_client_;
  LocalSessionEventHandler* handler_;
  syncer::SyncableService::StartSyncFlare flare_;

  DISALLOW_COPY_AND_ASSIGN(NotificationServiceSessionsRouter);
};

}  // namespace sync_sessions

#endif  // CHROME_BROWSER_SYNC_SESSIONS_NOTIFICATION_SERVICE_SESSIONS_ROUTER_H_
