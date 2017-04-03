// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_SYNC_SYNCED_TABS_SERVICE_H_
#define MOBILE_COMMON_SYNC_SYNCED_TABS_SERVICE_H_

#include <map>
#include <vector>

#include "base/memory/ref_counted.h"
#include "base/files/file_path.h"
#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "base/threading/non_thread_safe.h"
#include "base/threading/thread.h"
#include "base/threading/thread_checker.h"
#include "base/timer/timer.h"

#include "mobile/common/sync/synced_tab_data.h"

namespace mobile {

class SyncedTabsDatabase;

class SyncedTabsService : public base::NonThreadSafe {
 public:
  explicit SyncedTabsService(const base::FilePath& db_name);
  ~SyncedTabsService();

  int Lookup(SyncedTabData::id_type id);
  void Insert(SyncedTabData::id_type id, int syncId);
  void Update(SyncedTabData::id_type id, int syncId);
  void Delete(SyncedTabData::id_type id);

  void Flush();

 private:
  struct Entry {
    SyncedTabData::id_type id;
    int syncId;
    bool insert;
  };

  void WaitForCacheToLoad();

  void ScheduleUpdate();
  void DoScheduledUpdate();
  void ForceUpdate();

  void ScheduleTask(const base::Closure& task);

  static void DoUpdate(const scoped_refptr<SyncedTabsDatabase> db,
                       std::vector<Entry> queue);
  void DoLoad(const scoped_refptr<SyncedTabsDatabase> db);

  base::OneShotTimer timer_;
  base::ThreadChecker thread_checker_;
  base::Thread* thread_;
  scoped_refptr<SyncedTabsDatabase> db_;
  std::vector<Entry> queue_;

  base::Lock cache_lock_;
  base::ConditionVariable cache_not_loaded_;
  bool cache_loaded_;
  std::map<SyncedTabData::id_type, int> cache_;

  DISALLOW_COPY_AND_ASSIGN(SyncedTabsService);
};

}  // namespace mobile

#endif // MOBILE_COMMON_SYNC_SYNCED_TABS_SERVICE_H_
