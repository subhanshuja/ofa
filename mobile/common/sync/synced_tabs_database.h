// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_SYNC_SYNCED_TABS_DATABASE_H_
#define MOBILE_COMMON_SYNC_SYNCED_TABS_DATABASE_H_

#include <map>

#include "base/memory/ref_counted.h"
#include "base/files/file_path.h"

#include "mobile/common/sync/synced_tab_data.h"

namespace mobile {

class SyncedTabsDatabase :
      public base::RefCountedThreadSafe<SyncedTabsDatabase> {
 public:
  static const int kInvalidSyncId;

  static scoped_refptr<SyncedTabsDatabase> create();

  virtual bool Init(const base::FilePath& db_name) = 0;

  virtual void Load(std::map<SyncedTabData::id_type, int>* entries) = 0;
  virtual int Lookup(SyncedTabData::id_type id) = 0;
  virtual void Insert(SyncedTabData::id_type id, int syncId) = 0;
  virtual void Update(SyncedTabData::id_type id, int syncId) = 0;
  virtual void Delete(SyncedTabData::id_type id) = 0;

 protected:
  SyncedTabsDatabase();
  virtual ~SyncedTabsDatabase();

 private:
  friend class base::RefCountedThreadSafe<SyncedTabsDatabase>;

  DISALLOW_COPY_AND_ASSIGN(SyncedTabsDatabase);
};

}  // namespace mobile

#endif // MOBILE_COMMON_SYNC_SYNCED_TABS_DATABASE_H_
