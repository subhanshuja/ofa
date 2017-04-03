// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_SYNC_SYNCED_WINDOW_DELEGATE_H_
#define MOBILE_COMMON_SYNC_SYNCED_WINDOW_DELEGATE_H_

#include "base/memory/scoped_vector.h"
#include "base/files/file_path.h"
#include "components/sync_sessions/synced_window_delegate.h"

namespace sync_sessions {
class NotificationServiceSessionsRouter;
}

namespace mobile {

struct SyncedTabData;
class SyncedTabDelegate;
class SyncedTabsService;
class SyncManager;

class SyncedWindowDelegate : public sync_sessions::SyncedWindowDelegate {
 public:
  SyncedWindowDelegate(SyncManager* manager, const base::FilePath& db_name);
  virtual ~SyncedWindowDelegate();

  virtual void StartRestore();
  virtual void FinishRestore();
  virtual void InsertTab(int index, const SyncedTabData* data);
  virtual void RemoveTab(int index);
  virtual void UpdateTab(int index, const SyncedTabData* data);
  virtual void SetActiveTab(int index);

  SyncManager* GetManager() const {
    return manager_;
  }

  // Override sync_sessions::SyncedWindowDelegate
  bool HasWindow() const override;

  SessionID::id_type GetSessionId() const override;

  int GetTabCount() const override;

  int GetActiveIndex() const override;

  bool IsApp() const override;

  bool IsTypeTabbed() const override;

  bool IsTypePopup() const override;

  bool IsTabPinned(const sync_sessions::SyncedTabDelegate* tab) const override;

  sync_sessions::SyncedTabDelegate* GetTabAt(int index) const override;

  SessionID::id_type GetTabIdAt(int index) const override;

  bool IsSessionRestoreInProgress() const override;

  bool ShouldSync() const override;

  virtual void SetSessionsRouter(
      sync_sessions::NotificationServiceSessionsRouter* router);

  SyncedTabsService* GetSyncedTabsService() const;

  void Flush();

 protected:
  int active_;
  ScopedVector<SyncedTabDelegate> tabs_;

 private:
  SyncManager* manager_;
  sync_sessions::NotificationServiceSessionsRouter* router_;
  const SessionID session_;
  bool restore_active_;
  std::unique_ptr<SyncedTabsService> tabs_service_;
};

}  // namespace mobile

#endif /* MOBILE_COMMON_SYNC_SYNCED_WINDOW_DELEGATE_H_ */
