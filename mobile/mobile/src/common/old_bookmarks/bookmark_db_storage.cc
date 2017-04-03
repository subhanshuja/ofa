// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/old_bookmarks/bookmark_db_storage.h"

#include <string>
#include <queue>

#include "base/location.h"
#include "base/task_runner_util.h"
#include "base/threading/sequenced_worker_pool.h"
#include "sql/init_status.h"
#include "sql/meta_table.h"
#include "sql/statement.h"
#include "sql/transaction.h"

#include "common/old_bookmarks/bookmark_collection_impl.h"

#if defined(OS_MACOSX) && !defined(OS_IOS)
#include "base/mac/mac_util.h"
#endif

namespace opera {

const char* kBookmarkDatabaseFilename = "bookmarks.db";

namespace {

const char* DB_FAV_TABLE_NAME = "bookmarks";
const char* QUERY_CREATE_TABLE = "CREATE TABLE bookmarks ("
                                  "guid STRING NOT NULL,"
                                  "idx INTEGER NOT NULL,"
                                  "name STRING NOT NULL,"
                                  "url STRING,"
                                  "type INTEGER NOT NULL,"
                                  "parent_guid STRING,"
                                  "PRIMARY KEY (guid))";
const char* QUERY_CREATE_IDX_INDEX =
    "CREATE INDEX IF NOT EXISTS bookmarks_idx_index ON bookmarks(idx ASC)";
const char* QUERY_UPDATE =
    "INSERT OR REPLACE INTO bookmarks ("
    "guid, idx, name, url, type, parent_guid"
    ") VALUES (?, ?, ?, ?, ?, ?)";
const char* QUERY_GET_ROOT = "SELECT * FROM bookmarks "
                             "WHERE parent_guid IS NULL "
                             "ORDER BY idx ASC";
const char* QUERY_GET_CHILDREN = "SELECT * FROM bookmarks "
                                 "WHERE parent_guid=? "
                                 "ORDER BY idx ASC";
const char* QUERY_DELETE = "DELETE FROM bookmarks";

const int kDBStoragePageSize = (32 * 1024);  // 32k
const int kDBStorageCachedPages = 32;

}  // namespace

BookmarkDBStorage::BookmarkDBStorage(
    base::FilePath db_dir,
    scoped_refptr<base::SingleThreadTaskRunner> db_task_runner)
  : db_dir_(db_dir),
    db_task_runner_(db_task_runner),
    ready_to_use_(false) {
}

BookmarkDBStorage::~BookmarkDBStorage() {
  Release();
}

void BookmarkDBStorage::Load(const LoadedCallback& loaded) {
  if (db_service_) {
    loaded.Run(NULL);
    return;
  }

  InitLoading(loaded);
}

void BookmarkDBStorage::SaveBookmarkData(
    const scoped_refptr<BookmarkRawDataHolder>& data,
    const SavedCallback& callback) {
  if (!ready_to_use_)
    return;

  db_task_runner_->PostTaskAndReply(
      FROM_HERE,
      base::Bind(&DatabaseService::SaveBookmarkData, db_service_, data),
      base::Bind(&BookmarkDBStorage::OnDataSaved, AsWeakPtr(), callback));
}

void BookmarkDBStorage::SafeSaveBookmarkDataOnExit(
    const scoped_refptr<BookmarkRawDataHolder>& data) {
  if (!ready_to_use_)
    return;

  // TODO(felixe): This should *really* be done before we exit the application.
  // Previously, this was added to the browser threads pool of tasks to do on
  // shutdown, but we can't access those unless we include stuff from content/
  db_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&DatabaseService::SafeSaveBookmarkDataOnExit,
                 db_service_, data));
}

void BookmarkDBStorage::Stop() {
  if (db_service_)
    db_service_->Stop();
}

bool BookmarkDBStorage::IsStopped() const {
  return !db_service_ || db_service_->IsStopped();
}

void BookmarkDBStorage::Flush() {
  if (db_service_)
    db_service_->Flush();
}

void BookmarkDBStorage::Release() {
  ready_to_use_ = false;

  if (db_service_) {
    db_service_->AddRef();
    DatabaseService* raw_ptr = db_service_.get();
    db_service_ = NULL;

    db_task_runner_->ReleaseSoon(FROM_HERE, raw_ptr);
  }
}

void BookmarkDBStorage::InitLoading(const LoadedCallback& callback) {
  base::FilePath db_file(db_dir_.AppendASCII(kBookmarkDatabaseFilename));
  db_service_ = new DatabaseService(db_file);

  RootFolderHolder* holder = new RootFolderHolder();

  base::PostTaskAndReplyWithResult(
    db_task_runner_.get(),
    FROM_HERE,
    base::Bind(&DatabaseService::Load, db_service_, make_scoped_refptr(holder)),
    base::Bind(&BookmarkDBStorage::OnDataLoaded, AsWeakPtr(),
               callback,
               make_scoped_refptr(holder)));
}

void BookmarkDBStorage::OnDataLoaded(
    const LoadedCallback& callback,
    const scoped_refptr<RootFolderHolder>& root,
    bool result) {
  if (!result) {
    Release();
    callback.Run(NULL);
    return;
  }

  ready_to_use_ = true;
  callback.Run(root->root_folder_.release());
}

void BookmarkDBStorage::OnDataSaved(const SavedCallback& callback) {
  callback.Run();
}

BookmarkDBStorage::RootFolderHolder::RootFolderHolder() {
}

BookmarkDBStorage::RootFolderHolder::~RootFolderHolder() {
}

BookmarkDBStorage::DatabaseService::DatabaseService(
    const base::FilePath& db_path)
    : db_path_(db_path),
      force_save_if_stopped_(false) {
}

BookmarkDBStorage::DatabaseService::~DatabaseService() {
}

bool BookmarkDBStorage::DatabaseService::ContinueSave() const {
  return !IsStopped() || force_save_if_stopped_;
}

bool BookmarkDBStorage::DatabaseService::Load(
    const scoped_refptr<RootFolderHolder>& root) {
  if (db_.is_open())
    return true;

  db_.set_page_size(kDBStoragePageSize);
  db_.set_cache_size(kDBStorageCachedPages);
  db_.set_exclusive_locking();

  if (!db_.Open(db_path_))
    return false;

  sql::Transaction transaction(&db_);
  transaction.Begin();

#if defined(OS_MACOSX) && !defined(OS_IOS)
  // Exclude the favorites file from backups.
  base::mac::SetFileBackupExclusion(db_path_);
#endif

  bool result = false;
  do {
    sql::MetaTable meta_table;
    meta_table.Init(&db_,
                     kCurrentVersionNumber,
                     kCompatibleVersionNumber);

    if (!db_.DoesTableExist(DB_FAV_TABLE_NAME) &&
        !db_.Execute(QUERY_CREATE_TABLE)) {
      break;
    }

    if (!db_.Execute(QUERY_CREATE_IDX_INDEX))
      break;

    if (meta_table.GetCompatibleVersionNumber() > kCurrentVersionNumber) {
      LOG(WARNING) << "Bookmarks database is too new.";
      break;
    }

    result = false;

    if (!transaction.Commit())
      break;

    result = true;
  } while (0);

  if (!result) {
    transaction.Rollback();
    db_.Close();
    return false;
  }

  if (!root->root_folder_)
    root->root_folder_.reset(new BookmarkRootFolder());

  return LoadBookmarkData(root);
}

void BookmarkDBStorage::DatabaseService::SaveBookmarkData(
    const scoped_refptr<BookmarkRawDataHolder>& data) {
  DCHECK(db_.is_open());

  if (!ContinueSave())
    return;

  sql::Transaction transaction(&db_);
  transaction.Begin();

  Clear();

  for (size_t i = 0; i < data->size() && ContinueSave(); i++) {
    const BookmarkStorage::BookmarkRawDataHolder::BookmarkRawData&
        raw_data = (*data)[i];

    sql::Statement statement(db_.GetUniqueStatement(QUERY_UPDATE));
    statement.BindString(0, raw_data.guid_);
    statement.BindInt(1, raw_data.index_);
    statement.BindString16(2, raw_data.name_);

    if (raw_data.url_.size())
      statement.BindString(3, raw_data.url_);

    statement.BindInt(4, raw_data.type_);

    if (raw_data.parent_guid_.size())
      statement.BindString(5, raw_data.parent_guid_);

    statement.Run();
    if (!statement.Succeeded())
      break;
  }

  if (!ContinueSave() || !transaction.Commit())
    transaction.Rollback();
}

void BookmarkDBStorage::DatabaseService::SafeSaveBookmarkDataOnExit(
    const scoped_refptr<BookmarkRawDataHolder>& data) {
  force_save_if_stopped_ = true;
  SaveBookmarkData(data);
}

void BookmarkDBStorage::DatabaseService::Clear() {
  DCHECK(db_.is_open());

  ignore_result(db_.Execute(QUERY_DELETE));
}

bool BookmarkDBStorage::DatabaseService::LoadBookmarkData(
    const scoped_refptr<RootFolderHolder>& root) {
  std::queue<std::string> folder_queue;
  sql::Statement child_statement(
      db_.GetCachedStatement(sql::StatementID(QUERY_GET_ROOT), QUERY_GET_ROOT));

  while (true) {
    while (child_statement.Step()) {
      std::string guid = child_statement.ColumnString(0);
      base::string16 name = child_statement.ColumnString16(2);
      std::string url = child_statement.ColumnString(3);
      int type = child_statement.ColumnInt(4);
      std::string parent_guid = child_statement.ColumnString(5);

      if (!root->root_folder_->RecreateBookmark(guid, name, url, type,
                                                parent_guid))
        return false;

      if (type == Bookmark::BOOKMARK_FOLDER)
        folder_queue.push(guid);
    }

    if (folder_queue.empty())
      break;

    child_statement.Assign(db_.GetCachedStatement(
        sql::StatementID(QUERY_GET_CHILDREN), QUERY_GET_CHILDREN));
    child_statement.BindString(0, folder_queue.front());
    folder_queue.pop();
  }

  return true;
}

void BookmarkDBStorage::DatabaseService::Stop() {
  stopped_.Set();
}

bool BookmarkDBStorage::DatabaseService::IsStopped() const {
  return stopped_.IsSet();
}

void BookmarkDBStorage::DatabaseService::Flush() {
  // no-op, sqlite keeps database uptodate
}

}  // namespace opera
