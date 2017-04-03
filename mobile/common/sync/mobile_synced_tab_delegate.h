// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_SYNC_MOBILE_SYNCED_TAB_DELEGATE_H_
#define MOBILE_COMMON_SYNC_MOBILE_SYNCED_TAB_DELEGATE_H_

#include "components/sync_sessions/synced_tab_delegate.h"
#include "content/public/browser/navigation_entry.h"

namespace mobile {

struct SyncedTabData;
class SyncedWindowDelegate;

class SyncedTabDelegate : public sync_sessions::SyncedTabDelegate {
 public:
  SyncedTabDelegate(SyncedWindowDelegate* window, const SyncedTabData* data);
  virtual ~SyncedTabDelegate();

  virtual bool Update(const SyncedTabData* data);

  // Override browser_sync::SyncedTabDelegate
  SessionID::id_type GetWindowId() const override;
  SessionID::id_type GetSessionId() const override;

  bool IsBeingDestroyed() const override;

  std::string GetExtensionAppId() const override;

  bool IsInitialBlankNavigation() const override;
  int GetCurrentEntryIndex() const override;
  int GetEntryCount() const override;
  GURL GetVirtualURLAtIndex(int i) const override;
  GURL GetFaviconURLAtIndex(int i) const override;
  ui::PageTransition GetTransitionAtIndex(int i) const override;
  void GetSerializedNavigationAtIndex(
      int i,
      sessions::SerializedNavigationEntry* serialized_entry) const override;

  bool ProfileIsSupervised() const override;
  const std::vector<
      std::unique_ptr<const sessions::SerializedNavigationEntry>>*
  GetBlockedNavigations() const override;

  int GetSyncId() const override;
  void SetSyncId(int sync_id) override;
  bool ShouldSync(
      sync_sessions::SyncSessionsClient* sessions_client) override;

  bool IsPlaceholderTab() const override;

  virtual void Destroy();

 protected:
  virtual void UpdateEntry();

  std::unique_ptr<SyncedTabData> data_;
  std::unique_ptr<content::NavigationEntry> entry_;

 private:
  const SessionID id_;
  SyncedWindowDelegate* window_;
  int sync_id_;
  bool destroyed_;
};

}  // namespace mobile

#endif /* MOBILE_COMMON_SYNC_MOBILE_SYNCED_TAB_DELEGATE_H_ */
