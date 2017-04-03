// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/sync/synced_window_delegate.h"

#include <set>

#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/sync/sessions/notification_service_sessions_router.h"
#include "components/browser_sync/profile_sync_service.h"
#include "content/public/browser/notification_service.h"
#include "mobile/common/sync/mobile_synced_tab_delegate.h"
#include "mobile/common/sync/sync_manager.h"
#include "mobile/common/sync/synced_tabs_service.h"

#if defined(OS_ANDROID)
#include "chrome/browser/sync/glue/synced_window_delegates_getter_android.h"
#else
#include "chrome/browser/ui/sync/browser_synced_window_delegates_getter.h"
#endif  // OS_ANDROID

namespace {

std::set<const sync_sessions::SyncedWindowDelegate*> g_delegates;

}  // namespace

namespace mobile {

SyncedWindowDelegate::SyncedWindowDelegate(
    SyncManager* manager, const base::FilePath& db_name)
    : active_(-1),
      manager_(manager),
      router_(nullptr),
      restore_active_(false),
      tabs_service_(new SyncedTabsService(db_name)) {
  DCHECK(manager_);
  g_delegates.insert(this);
}

SyncedWindowDelegate::~SyncedWindowDelegate() {
  g_delegates.erase(this);
}

bool SyncedWindowDelegate::HasWindow() const {
  return true;
}

SessionID::id_type SyncedWindowDelegate::GetSessionId() const {
  return session_.id();
}

int SyncedWindowDelegate::GetTabCount() const {
  return tabs_.size();
}

int SyncedWindowDelegate::GetActiveIndex() const {
  return active_;
}

bool SyncedWindowDelegate::IsApp() const {
  return false;
}

bool SyncedWindowDelegate::IsTypeTabbed() const {
  return true;
}

bool SyncedWindowDelegate::IsTypePopup() const {
  return false;
}

bool SyncedWindowDelegate::IsTabPinned(
    const sync_sessions::SyncedTabDelegate* tab) const {
  return false;
}

sync_sessions::SyncedTabDelegate*
SyncedWindowDelegate::GetTabAt(int index) const {
  if (index < 0 || index >= static_cast<int>(tabs_.size()))
    return nullptr;
  return tabs_[index];
}

SessionID::id_type SyncedWindowDelegate::GetTabIdAt(int index) const {
  CHECK(index >= 0 && index < static_cast<int>(tabs_.size()));
  return tabs_[index]->GetSessionId();
}

bool SyncedWindowDelegate::IsSessionRestoreInProgress() const {
  return restore_active_;
}

bool SyncedWindowDelegate::ShouldSync() const {
  return true;
}

void SyncedWindowDelegate::StartRestore() {
  DCHECK(!restore_active_);
  restore_active_ = true;
}

void SyncedWindowDelegate::FinishRestore() {
  DCHECK(restore_active_);
  restore_active_ = false;
  ProfileSyncServiceFactory::GetForProfile(
      manager_->GetProfile())->OnSessionRestoreComplete();
}

void SyncedWindowDelegate::SetActiveTab(int index) {
  if (index >= 0 && index < static_cast<int>(tabs_.size())) {
    active_ = index;
    if (router_)
      router_->SignalTabChange(tabs_[active_]);
  } else {
    active_ = -1;
  }
}

void SyncedWindowDelegate::InsertTab(int index, const SyncedTabData* data) {
  if (index < 0 || index > static_cast<int>(tabs_.size())) {
    DLOG(INFO) << "InsertTab bad index: " << index;
    DCHECK(false);
    return;
  }
  SyncedTabDelegate* tab = new SyncedTabDelegate(this, data);
  tabs_.insert(tabs_.begin() + index, tab);
  if (router_) router_->SignalTabChange(tab);
}

void SyncedWindowDelegate::RemoveTab(int index) {
  if (index < 0 || index >= static_cast<int>(tabs_.size())) {
    DLOG(INFO) << "RemoveTab bad index: " << index;
    DCHECK(false);
    return;
  }
  ScopedVector<SyncedTabDelegate>::iterator i(tabs_.begin() + index);
  std::unique_ptr<SyncedTabDelegate> tab(*i);
  tabs_.weak_erase(i);
  tab->Destroy();
  if (router_) router_->SignalTabChange(tab.get());
}

void SyncedWindowDelegate::UpdateTab(int index, const SyncedTabData* data) {
  if (index < 0 || index >= static_cast<int>(tabs_.size())) {
    DLOG(INFO) << "UpdateTab bad index: " << index;
    DCHECK(false);
    return;
  }
  if (tabs_[index]->Update(data)) {
    if (router_) router_->SignalTabChange(tabs_[index]);
  }
}

void SyncedWindowDelegate::SetSessionsRouter(
    sync_sessions::NotificationServiceSessionsRouter* router) {
  router_ = router;
}

SyncedTabsService* SyncedWindowDelegate::GetSyncedTabsService() const {
  return tabs_service_.get();
}

void SyncedWindowDelegate::Flush() {
  tabs_service_->Flush();
}

}  // namespace mobile

namespace browser_sync {

#if defined(OS_ANDROID)
SyncedWindowDelegatesGetterAndroid::SyncedWindowDelegatesGetterAndroid() {
}

SyncedWindowDelegatesGetterAndroid::~SyncedWindowDelegatesGetterAndroid() {
}

std::set<const sync_sessions::SyncedWindowDelegate*>
SyncedWindowDelegatesGetterAndroid::GetSyncedWindowDelegates() {
  return g_delegates;
}

const sync_sessions::SyncedWindowDelegate*
SyncedWindowDelegatesGetterAndroid::FindById(SessionID::id_type id) {
  for (auto& delegate : g_delegates) {
    if (delegate->GetSessionId() == id) {
      return delegate;
    }
  }
  return nullptr;
}
#else
BrowserSyncedWindowDelegatesGetter::BrowserSyncedWindowDelegatesGetter(
    Profile* profile)
    : profile_(profile) {
}

BrowserSyncedWindowDelegatesGetter::~BrowserSyncedWindowDelegatesGetter() {
}

std::set<const SyncedWindowDelegate*>
BrowserSyncedWindowDelegatesGetter::GetSyncedWindowDelegates() {
  return g_delegates;
}

const SyncedWindowDelegate*
BrowserSyncedWindowDelegatesGetter::FindById(SessionID::id_type id) {
  for (auto& delegate : g_delegates) {
    if (delegate->GetSessionId() == id) {
      return delegate;
    }
  }
  return nullptr;
}
#endif  // OS_ANDROID

}  // namespace browser_sync
