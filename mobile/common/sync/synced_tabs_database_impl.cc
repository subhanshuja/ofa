// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/sync/synced_tabs_database_impl.h"

#include "sql/statement.h"
#include "sql/transaction.h"

namespace {

const int kCurrentVersionNumber = 1;
const int kCompatibleVersionNumber = 1;

bool InitTables(sql::Connection* db) {
  const char kSyncIdsSql[] =
      "CREATE TABLE IF NOT EXISTS sync_ids"
      "("
#ifdef OS_IOS
      "id TEXT,"
#else
      "id INTEGER,"
#endif
      "sync_id INTEGER,"
      "PRIMARY KEY (id, sync_id)"
      ")";
  if (!db->Execute(kSyncIdsSql))
    return false;

  return true;
}

void BindId(
    sql::Statement* statement, int column, mobile::SyncedTabData::id_type id) {
#ifdef OS_IOS
  statement->BindString(column, id);
#else
  statement->BindInt(column, id);
#endif
}

mobile::SyncedTabData::id_type ColumnId(sql::Statement* statement, int column) {
#ifdef OS_IOS
  return statement->ColumnString(column);
#else
  return statement->ColumnInt(column);
#endif
}

}  // namespace

namespace mobile {

// static
scoped_refptr<SyncedTabsDatabase>
SyncedTabsDatabase::create() {
  return scoped_refptr<SyncedTabsDatabase>(new SyncedTabsDatabaseImpl());
}

SyncedTabsDatabaseImpl::SyncedTabsDatabaseImpl() {
}

SyncedTabsDatabaseImpl::~SyncedTabsDatabaseImpl() {
}

void SyncedTabsDatabaseImpl::Load(
    std::map<SyncedTabData::id_type, int>* entries) {
  CHECK(entries);
  sql::Statement statement(db_.GetCachedStatement(SQL_FROM_HERE,
      "SELECT id,sync_id FROM sync_ids"));
  entries->clear();
  while (statement.Step()) {
    entries->insert(std::make_pair(ColumnId(&statement, 0),
                                   statement.ColumnInt(1)));
  }
}

int SyncedTabsDatabaseImpl::Lookup(SyncedTabData::id_type id) {
  sql::Statement statement(db_.GetCachedStatement(SQL_FROM_HERE,
      "SELECT sync_id FROM sync_ids WHERE id=?"));
  BindId(&statement, 0, id);
  if (statement.Step()) {
    return statement.ColumnInt(0);
  }
  return kInvalidSyncId;
}

void SyncedTabsDatabaseImpl::Insert(SyncedTabData::id_type id, int syncId) {
  sql::Statement statement(db_.GetCachedStatement(SQL_FROM_HERE,
      "INSERT INTO sync_ids (id, sync_id) VALUES(?, ?)"));
  BindId(&statement, 0, id);
  statement.BindInt(1, syncId);
  statement.Run();
}

void SyncedTabsDatabaseImpl::Update(SyncedTabData::id_type id, int syncId) {
  sql::Statement statement(db_.GetCachedStatement(SQL_FROM_HERE,
      "UPDATE sync_ids SET sync_id=? WHERE id=?"));
  statement.BindInt(0, syncId);
  BindId(&statement, 1, id);
  statement.Run();
}

void SyncedTabsDatabaseImpl::Delete(SyncedTabData::id_type id) {
  sql::Statement statement(db_.GetCachedStatement(SQL_FROM_HERE,
      "DELETE FROM sync_ids WHERE id=?"));
  BindId(&statement, 0, id);
  statement.Run();
}

bool SyncedTabsDatabaseImpl::Init(const base::FilePath& db_name) {
  // Run the database in exclusive mode. Nobody else should be accessing the
  // database while we're running, and this will give somewhat improved perf.
  db_.set_exclusive_locking();

  if (!db_.Open(db_name))
    return false;

  sql::Transaction transaction(&db_);
  if (!transaction.Begin())
    return false;

    // Create the tables.
  if (!meta_table_.Init(&db_, kCurrentVersionNumber,
                        kCompatibleVersionNumber) ||
      !InitTables(&db_)) {
    return false;
  }

  if (meta_table_.GetCompatibleVersionNumber() > kCurrentVersionNumber) {
    LOG(WARNING) << "Synced tabs database is too new.";
    return false;
  }

  if (!transaction.Commit())
    return false;

  return true;
}

}  // namespace mobile
