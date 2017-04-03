// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_SYNC_SYNCED_TABS_DATABASE_IMPL_H_
#define MOBILE_COMMON_SYNC_SYNCED_TABS_DATABASE_IMPL_H_

#include "mobile/common/sync/synced_tabs_database.h"

#include "sql/connection.h"
#include "sql/meta_table.h"

namespace mobile {

class SyncedTabsDatabaseImpl : public SyncedTabsDatabase {
 public:
  SyncedTabsDatabaseImpl();

  bool Init(const base::FilePath& db_name) override;

  void Load(std::map<SyncedTabData::id_type, int>* entries) override;
  int Lookup(SyncedTabData::id_type id) override;
  void Insert(SyncedTabData::id_type id, int syncId) override;
  void Update(SyncedTabData::id_type id, int syncId) override;
  void Delete(SyncedTabData::id_type id) override;

 private:
  virtual ~SyncedTabsDatabaseImpl();

  sql::Connection db_;
  sql::MetaTable meta_table_;
};

}  // namespace mobile

#endif // MOBILE_COMMON_SYNC_SYNCED_TABS_DATABASE_IMPL_H_
