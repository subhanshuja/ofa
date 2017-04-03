// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/sync/mobile_synced_tab_delegate.h"

#include "base/time/time.h"
#include "chrome/browser/profiles/profile.h"
#include "components/sessions/content/content_serialized_navigation_builder.h"
#include "components/sync_sessions/sync_sessions_client.h"
#include "components/sync_sessions/synced_window_delegates_getter.h"
#include "mobile/common/sync/sync_manager.h"
#include "mobile/common/sync/synced_tab_data.h"
#include "mobile/common/sync/synced_tabs_service.h"
#include "mobile/common/sync/synced_window_delegate.h"

namespace mobile {

SyncedTabDelegate::SyncedTabDelegate(SyncedWindowDelegate* window,
                                     const SyncedTabData* data)
    : data_(new SyncedTabData(*data)),
      entry_(content::NavigationEntry::Create()),
      window_(window),
      sync_id_(-1),
      destroyed_(false) {
  sync_id_ = window_->GetSyncedTabsService()->Lookup(data_->id);
  UpdateEntry();
}

SyncedTabDelegate::~SyncedTabDelegate() {
}

void SyncedTabDelegate::Destroy() {
  if (window_ && sync_id_ != -1) {
    window_->GetSyncedTabsService()->Delete(data_->id);
  }
  destroyed_ = true;
  window_ = nullptr;
}

bool SyncedTabDelegate::Update(const SyncedTabData* data) {
  const SyncedTabData* old = data_.get();
  if (*old == *data)
    return false;
  data_.reset(new SyncedTabData(*data));
  if (window_) {
    sync_id_ = window_->GetSyncedTabsService()->Lookup(data_->id);
  }
  UpdateEntry();
  return true;
}

SessionID::id_type SyncedTabDelegate::GetWindowId() const {
  return window_ ? window_->GetSessionId() : -1;
}

SessionID::id_type SyncedTabDelegate::GetSessionId() const {
  return id_.id();
}

bool SyncedTabDelegate::IsBeingDestroyed() const {
  return destroyed_;
}

std::string SyncedTabDelegate::GetExtensionAppId() const {
  return "";
}

bool SyncedTabDelegate::IsInitialBlankNavigation() const {
  return false;
}

int SyncedTabDelegate::GetCurrentEntryIndex() const {
  return 0;
}

int SyncedTabDelegate::GetEntryCount() const {
  return 1;
}

GURL SyncedTabDelegate::GetVirtualURLAtIndex(int i) const {
  auto entry = i == 0 ? entry_.get() : nullptr;
  return entry ? entry->GetVirtualURL() : GURL();
}

GURL SyncedTabDelegate::GetFaviconURLAtIndex(int i) const {
  return GURL();
}

ui::PageTransition SyncedTabDelegate::GetTransitionAtIndex(int i) const {
  return ui::PAGE_TRANSITION_LINK;
}

void SyncedTabDelegate::GetSerializedNavigationAtIndex(
    int i,
    sessions::SerializedNavigationEntry* serialized_entry) const {
  auto entry = i == 0 ? entry_.get() : nullptr;
  *serialized_entry =
      sessions::ContentSerializedNavigationBuilder::FromNavigationEntry(i,
                                                                        *entry);
}

bool SyncedTabDelegate::ProfileIsSupervised() const {
  return false;
}

const std::vector<
    std::unique_ptr<const sessions::SerializedNavigationEntry>>*
SyncedTabDelegate::GetBlockedNavigations() const {
  return nullptr;
}

int SyncedTabDelegate::GetSyncId() const {
  return sync_id_;
}

void SyncedTabDelegate::SetSyncId(int sync_id) {
  DCHECK(window_);
  if (sync_id_ != sync_id) {
    if (window_) {
      if (sync_id_ == -1) {
        window_->GetSyncedTabsService()->Insert(data_->id, sync_id);
      } else {
        window_->GetSyncedTabsService()->Update(data_->id, sync_id);
      }
    }
    sync_id_ = sync_id;
  }
}

bool SyncedTabDelegate::ShouldSync(
    sync_sessions::SyncSessionsClient* sessions_client) {
  if (sessions_client->GetSyncedWindowDelegatesGetter()->FindById(
          GetWindowId()) == nullptr)
    return false;

  return true;
}

bool SyncedTabDelegate::IsPlaceholderTab() const {
  return false;
}

void SyncedTabDelegate::UpdateEntry() {
  entry_->SetTimestamp(base::Time::Now());
  entry_->SetURL(data_->url);
  entry_->SetVirtualURL(data_->visible_url);
  entry_->SetOriginalRequestURL(data_->original_request_url);
  entry_->SetTitle(data_->title);
}

}  // namespace mobile
