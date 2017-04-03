// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/sessions/notification_service_sessions_router.h"

#include "base/logging.h"
#include "chrome/browser/sync/glue/sync_start_util.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/ui/sync/browser_synced_window_delegates_getter.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/sync_sessions/sync_sessions_client.h"
#include "components/sync_sessions/synced_tab_delegate.h"
#include "mobile/common/sync/synced_window_delegate.h"

namespace sync_sessions {

NotificationServiceSessionsRouter::NotificationServiceSessionsRouter(
    Profile* profile,
    sync_sessions::SyncSessionsClient* sessions_client,
    const syncer::SyncableService::StartSyncFlare& flare)
    : profile_(profile), sessions_client_(sessions_client), handler_(nullptr), flare_(flare) {}

NotificationServiceSessionsRouter::~NotificationServiceSessionsRouter() {}

void NotificationServiceSessionsRouter::SignalTabChange(
    SyncedTabDelegate* tab) {
  if (handler_)
    handler_->OnLocalTabModified(tab);
  if (!tab->ShouldSync(sessions_client_))
    return;
  if (!flare_.is_null()) {
    flare_.Run(syncer::SESSIONS);
    flare_.Reset();
  }
}

void NotificationServiceSessionsRouter::StartRoutingTo(
    LocalSessionEventHandler* handler) {
  DCHECK(!handler_);
  handler_ = handler;

  for (auto& delegate : ProfileSyncServiceFactory::GetForProfile(profile_)
                            ->sync_client()->GetSyncSessionsClient()
                            ->GetSyncedWindowDelegatesGetter()
                            ->GetSyncedWindowDelegates()) {
    static_cast<mobile::SyncedWindowDelegate*>(
        const_cast<SyncedWindowDelegate*>(delegate))
        ->SetSessionsRouter(this);
  }
}

void NotificationServiceSessionsRouter::Stop() {
  if (!handler_)
    return;

  for (auto& delegate : ProfileSyncServiceFactory::GetForProfile(profile_)
                            ->sync_client()->GetSyncSessionsClient()
                            ->GetSyncedWindowDelegatesGetter()
                            ->GetSyncedWindowDelegates()) {
    static_cast<mobile::SyncedWindowDelegate*>(
        const_cast<SyncedWindowDelegate*>(delegate))
        ->SetSessionsRouter(nullptr);
  }

  handler_ = nullptr;
}

}  // namespace sync_sessions
