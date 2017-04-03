// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/favorites/favorite_db_storage.h"

#include <string>
#include <vector>

#include "base/location.h"
#include "base/task_runner_util.h"
#include "base/threading/sequenced_worker_pool.h"

#include "sql/init_status.h"
#include "sql/meta_table.h"
#include "sql/statement.h"
#include "sql/transaction.h"

#include "common/favorites/favorite_collection_impl.h"

#if defined(OS_MACOSX) && !defined(OS_IOS)
#include "base/mac/mac_util.h"
#endif

namespace opera {

namespace {

const char* DB_FILENAME = "favorites.db";
const char* DB_FAV_TABLE_NAME = "favorites";
const char* QUERY_CREATE_TABLE = "CREATE TABLE favorites ("
                                  "guid STRING NOT NULL,"
                                  "idx INTEGER NOT NULL,"
                                  "name STRING NOT NULL,"
                                  "url STRING,"
                                  "type INTEGER NOT NULL,"
                                  "parent_guid STRING,"
                                  "serialized_data BLOB,"
                                  "PRIMARY KEY (guid))";
const char* QUERY_UPDATE =
    "INSERT OR REPLACE INTO favorites ("
    "guid, idx, name, url, type, parent_guid, serialized_data"
    ") VALUES (?, ?, ?, ?, ?, ?, ?)";
const char* QUERY_GET_ROOT = "SELECT * FROM favorites "
                             "WHERE parent_guid IS NULL "
                             "ORDER BY idx ASC";
const char* QUERY_GET_CHILDREN = "SELECT * FROM favorites "
                                 "WHERE parent_guid IS NOT NULL "
                                 "ORDER BY parent_guid ASC, idx ASC";
const char* QUERY_DELETE = "DELETE FROM favorites";
const char* QUERY_GET_ROOT_FOLDER_GUIDS = "SELECT guid FROM favorites "
                                          "WHERE parent_guid IS NULL "
                                          "AND type = 1";
const char* QUERY_COUNT_FOLDERS_CHILDREN = "SELECT COUNT(*) FROM favorites "
                                           "WHERE parent_guid = ?";
const char* QUERY_DELETE_BY_GUID = "DELETE FROM favorites WHERE guid = ?";
const char* QUERY_SELECT_ALL_FAVORITES = "SELECT * FROM favorites";
const char* QUERY_UPDATE_FAVORITE_DATA =
    "UPDATE favorites SET serialized_data = ? WHERE guid = ?";

Favorite::Type DBTypeToFavoriteType(int db_type) {
  switch (db_type) {
    case 0:
    case 3:
      return Favorite::kFavoriteSite;
    case 1:
      return Favorite::kFavoriteFolder;
    case 2:
      return Favorite::kFavoriteExtension;
    default: {
      const Favorite::Type type = Favorite::Type();
      DLOG(WARNING) << "Unexpected Favorite type in the DB. Assuming" << type;
      return type;
    }
  }
}

int FavoriteTypeToDBType(Favorite::Type type) {
  // The mappings must not change unless the DB version changes as well.
  switch (type) {
    case Favorite::kFavoriteSite:
      return 0;
    case Favorite::kFavoriteFolder:
      return 1;
    case Favorite::kFavoriteExtension:
      return 2;
    default:
      NOTREACHED() << "Unexpected Favorite type";
      return -1;
  }
}

void MakeFavoriteDataVersionNumbered(
    std::vector<std::pair<std::string, std::string> >* data) {
  for (std::vector<std::pair<std::string, std::string> >::iterator it
       = data->begin();
       it != data->end();
       it++) {
    // Deserialize blob without version number
    base::Pickle pickle(it->second.c_str(), it->second.length());
    base::PickleIterator iterator(pickle);
    FavoriteData fd;
    fd.DeserializeVersion0(&iterator);

    // Serialize the data using version number
    base::Pickle new_format;
    fd.Serialize(&new_format);

    // Update the data in the mapping to the version-numbered version ;-)
    it->second = std::string(reinterpret_cast<const char*>(new_format.data()),
                             new_format.size());
  }
}

}  // namespace

FavoriteDBStorage::FavoriteDBStorage(
    base::FilePath db_path,
    scoped_refptr<base::SingleThreadTaskRunner> db_task_runner)
  : db_path_(db_path), db_task_runner_(db_task_runner), ready_to_use_(false) {
}

FavoriteDBStorage::~FavoriteDBStorage() {
  Release();
}

void FavoriteDBStorage::Load(const LoadedCallback& loaded) {
  VLOG(3) << "SYNC: FavoriteDBStorage::Load";
  if (db_service_) {
    loaded.Run(NULL);
    return;
  }

  InitLoading(loaded);
}

void FavoriteDBStorage::SaveFavoriteData(
    const scoped_refptr<FavoriteRawDataHolder>& data,
    const SavedCallback& callback) {
  if (!ready_to_use_)
    return;

  db_task_runner_->PostTaskAndReply(
      FROM_HERE,
      base::Bind(&DatabaseService::SaveFavoriteData, db_service_, data),
      base::Bind(&FavoriteDBStorage::OnDataSaved, AsWeakPtr(), callback));
}

void FavoriteDBStorage::SafeSaveFavoriteDataOnExit(
    const scoped_refptr<FavoriteRawDataHolder>& data) {
  if (!ready_to_use_)
    return;

  // TODO(felixe): There's nothing safe about exiting on Android atm... We
  // should add a PleaseWriteToDiskSynchronouslyNow() method or something at
  // least
  db_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&DatabaseService::SafeSaveFavoriteDataOnExit,
                 db_service_, data));
}

void FavoriteDBStorage::Stop() {
  if (db_service_)
    db_service_->Stop();
}

bool FavoriteDBStorage::IsStopped() const {
  return !db_service_ || db_service_->IsStopped();
}

void FavoriteDBStorage::Flush() {
  if (db_service_)
    db_service_->Flush();
}

void FavoriteDBStorage::Release() {
  ready_to_use_ = false;

  if (db_service_) {
    db_service_->AddRef();
    DatabaseService* raw_ptr = db_service_.get();
    db_service_ = NULL;

    db_task_runner_->ReleaseSoon(FROM_HERE, raw_ptr);
  }
}

void FavoriteDBStorage::InitLoading(const LoadedCallback& callback) {
  base::FilePath db_file = db_path_.AppendASCII(DB_FILENAME);
  db_service_ = new DatabaseService(db_file);

  RootFolderHolder* holder = new RootFolderHolder();

  base::PostTaskAndReplyWithResult(
    db_task_runner_.get(),
    FROM_HERE,
    base::Bind(&DatabaseService::Load, db_service_, make_scoped_refptr(holder)),
    base::Bind(&FavoriteDBStorage::OnDataLoaded, AsWeakPtr(),
               callback,
               make_scoped_refptr(holder)));
}

void FavoriteDBStorage::OnDataLoaded(
    const LoadedCallback& callback,
    const scoped_refptr<RootFolderHolder>& root,
    bool result) {
  VLOG(3) << "SYNC: FavoriteDBStorage::OnDataLoaded";
  if (!result) {
    Release();
    callback.Run(NULL);
    return;
  }

  ready_to_use_ = true;
  callback.Run(root->root_folder_.release());
}

void FavoriteDBStorage::OnDataSaved(const SavedCallback& callback) {
  callback.Run();
}

FavoriteDBStorage::RootFolderHolder::RootFolderHolder() {
}

FavoriteDBStorage::RootFolderHolder::~RootFolderHolder() {
}

#define DBSTORAGE_PAGE_SIZE (32*1024)  // 32k
#define DBSTORE_CACHED_PAGES 32

FavoriteDBStorage::DatabaseService::DatabaseService(
    const base::FilePath& db_path)
    : db_path_(db_path),
      force_save_if_stopped_(false) {
}

FavoriteDBStorage::DatabaseService::~DatabaseService() {
}

bool FavoriteDBStorage::DatabaseService::UpgradeToVersion3() {
  return DeleteEmptyFolders() && UpgradeFavoriteDataFormat();
}


bool FavoriteDBStorage::DatabaseService::ContinueSave() const {
  return !IsStopped() || force_save_if_stopped_;
}

void FavoriteDBStorage::DatabaseService::GetFavoriteData(
    GuidToDataMapping* data) {
  sql::Statement select_all(
      db_.GetUniqueStatement(QUERY_SELECT_ALL_FAVORITES));

  while (select_all.Step()) {
    std::string guid = select_all.ColumnString(0);

    std::string serialized_data;
    select_all.ColumnBlobAsString(6, &serialized_data);

    data->push_back(std::make_pair(guid, serialized_data));
  }
}

bool FavoriteDBStorage::DatabaseService::WriteFavoriteData(
    const GuidToDataMapping& data) {
  for (GuidToDataMapping::const_iterator it = data.begin();
       it != data.end();
       it++) {
    sql::Statement update_favorite_data(
        db_.GetUniqueStatement(QUERY_UPDATE_FAVORITE_DATA));

    update_favorite_data.BindString(0, it->second);
    update_favorite_data.BindString(1, it->first);

    update_favorite_data.Run();

    if (!update_favorite_data.Succeeded())
      return false;
  }

  return true;
}

void FavoriteDBStorage::DatabaseService::GetEmptyFoldersGuid(
    std::vector<std::string>* guids) {
  sql::Statement statement(db_.GetUniqueStatement(QUERY_GET_ROOT_FOLDER_GUIDS));

  while (statement.Step()) {
    guids->push_back(statement.ColumnString(0));
  }
}

bool FavoriteDBStorage::DatabaseService::GetFoldersChildCount(
    const std::string& guid, int* nr_of_children) {
  sql::Statement count_children(
      db_.GetUniqueStatement(QUERY_COUNT_FOLDERS_CHILDREN));

  count_children.BindString(0, guid);

  if (count_children.Step()) {
    *nr_of_children = count_children.ColumnInt(0);
    return true;
  }

  return false;
}

bool FavoriteDBStorage::DatabaseService::UpgradeFavoriteDataFormat() {
  GuidToDataMapping data;

  GetFavoriteData(&data);
  MakeFavoriteDataVersionNumbered(&data);

  return WriteFavoriteData(data);
}

bool FavoriteDBStorage::DatabaseService::DeleteEmptyFolders() {
  std::vector<std::string> empty_folder_guids;
  GetEmptyFoldersGuid(&empty_folder_guids);

  for (std::vector<std::string>::const_iterator it = empty_folder_guids.begin();
       it != empty_folder_guids.end();
       it++) {
    int nr_of_children = 0;
    if (!GetFoldersChildCount(*it, &nr_of_children))
      return false;

    if (nr_of_children == 0) {
      if (!DeleteFavorite(*it)) {
        return false;
      }
    }
  }

  return true;
}

bool FavoriteDBStorage::DatabaseService::DeleteFavorite(
    const std::string& guid) {
  sql::Statement delete_folder(db_.GetUniqueStatement(QUERY_DELETE_BY_GUID));
  delete_folder.BindString(0, guid);

  delete_folder.Run();

  return delete_folder.Succeeded();
}

bool FavoriteDBStorage::DatabaseService::Load(
    const scoped_refptr<RootFolderHolder>& root) {
  if (db_.is_open())
    return true;

  db_.set_page_size(DBSTORAGE_PAGE_SIZE);
  db_.set_cache_size(DBSTORE_CACHED_PAGES);
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

    if (meta_table.GetCompatibleVersionNumber() > kCurrentVersionNumber) {
      LOG(WARNING) << "Favorites database is too new.";
      break;
    }

    result = false;
    int cur_version = meta_table.GetVersionNumber();
    if (cur_version == 2) {
      if (UpgradeToVersion3())
        meta_table.SetVersionNumber(3);
      else
        break;
    }

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
    root->root_folder_.reset(new FavoriteRootFolder());

  return LoadSelected(QUERY_GET_ROOT, root) &&
         LoadSelected(QUERY_GET_CHILDREN, root);
}

void FavoriteDBStorage::DatabaseService::SaveFavoriteData(
    const scoped_refptr<FavoriteRawDataHolder>& data) {
  DCHECK(db_.is_open());

  if (!ContinueSave())
    return;

  sql::Transaction transaction(&db_);
  transaction.Begin();

  Clear();

  for (size_t i = 0; i < data->size() && ContinueSave(); i++) {
    const FavoriteStorage::FavoriteRawDataHolder::FavoriteRawData&
        raw_data = (*data)[i];

    sql::Statement statement(db_.GetUniqueStatement(QUERY_UPDATE));
    statement.BindString(0, raw_data.guid_);
    statement.BindInt(1, raw_data.index_);
    statement.BindString16(2, raw_data.name_);

    if (raw_data.url_.size())
      statement.BindString(3, raw_data.url_);

    statement.BindInt(4, FavoriteTypeToDBType(Favorite::Type(raw_data.type_)));

    if (raw_data.parent_guid_.size())
      statement.BindString(5, raw_data.parent_guid_);

    if (raw_data.serialized_data_.size())
      statement.BindBlob(6,
                         raw_data.serialized_data_.data(),
                         raw_data.serialized_data_.size());
    else
      statement.BindBlob(6, NULL, 0);

    statement.Run();
    if (!statement.Succeeded())
      break;
  }

  if (!ContinueSave() || !transaction.Commit())
    transaction.Rollback();
}

void FavoriteDBStorage::DatabaseService::SafeSaveFavoriteDataOnExit(
    const scoped_refptr<FavoriteRawDataHolder>& data) {
  force_save_if_stopped_ = true;
  SaveFavoriteData(data);
}

void FavoriteDBStorage::DatabaseService::Clear() {
  DCHECK(db_.is_open());

  ignore_result(db_.Execute(QUERY_DELETE));
}

bool FavoriteDBStorage::DatabaseService::LoadSelected(
    const char* query,
    const scoped_refptr<RootFolderHolder>& root) {
  sql::Statement child_statement(
      db_.GetCachedStatement(sql::StatementID(query), query));

  while (child_statement.Step()) {
    std::vector<char> serialization_data;
    child_statement.ColumnBlobAsVector(6, &serialization_data);
    base::Pickle pickle(
        serialization_data.size() ? &serialization_data.front() : NULL,
        serialization_data.size());


    if (!root->root_folder_->RecreateFavorite(
          child_statement.ColumnString(0),
          child_statement.ColumnString16(2),
          child_statement.ColumnString(3),
          DBTypeToFavoriteType(child_statement.ColumnInt(4)),
          child_statement.ColumnString(5),
          pickle)) {
      return false;
    }
  }
  return true;
}

void FavoriteDBStorage::DatabaseService::Stop() {
  stopped_.Set();
}

bool FavoriteDBStorage::DatabaseService::IsStopped() const {
  return stopped_.IsSet();
}

void FavoriteDBStorage::DatabaseService::Flush() {
  // no-op, sqlite keeps database uptodate
}

}  // namespace opera
