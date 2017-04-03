// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_FAVORITES_FAVORITE_DB_STORAGE_H_
#define COMMON_FAVORITES_FAVORITE_DB_STORAGE_H_

#include <string>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/cancellation_flag.h"

#include "sql/connection.h"

#include "common/favorites/favorite.h"
#include "common/favorites/favorite_storage.h"

class Profile;

namespace opera {

/**
 * Favorites persistent storage backend implemented with usage of SQLite .
 */
class FavoriteDBStorage
  : public FavoriteStorage,
    public base::SupportsWeakPtr<FavoriteDBStorage> {
 public:
  explicit FavoriteDBStorage(
      base::FilePath db_path,
      scoped_refptr<base::SingleThreadTaskRunner> db_task_runner);

  /**
   * Load the favorites from persistent storage.
   * This method will create new FavoriteRootFolder and pollute it with
   * data fetched from persistent storage. Once loaded callback is called,
   * it should take ownership of new_root passed to it.
   */
  void Load(const LoadedCallback& loaded) override;

    /**
   * Add or update favorite data in persistent storage.
   */
  virtual void SaveFavoriteData(
      const scoped_refptr<FavoriteRawDataHolder>& data,
      const SavedCallback& callback) override;

  virtual void SafeSaveFavoriteDataOnExit(
      const scoped_refptr<FavoriteRawDataHolder>& data) override;

  /**
   * Cancel any pending save.
   */
  void Stop() override;

  /**
   * Is storage shutting down?
   */
  bool IsStopped() const override;

  void Flush() override;

 private:
  /**
   * RootFolderHolder is a thread safe ref counted wrapper on
   * FavoriteRootFolder. It is used in order to safely pass pointer
   * to FavoriteRootFolder between threads.
   */
  class RootFolderHolder
    : public base::RefCountedThreadSafe<RootFolderHolder> {
  public:
    RootFolderHolder();
    std::unique_ptr<FavoriteRootFolder> root_folder_;

  private:
    friend class base::RefCountedThreadSafe<RootFolderHolder>;
    ~RootFolderHolder();
  };

  /**
   * DatabaseService class.
   * Implement DB operations on favorites DB. Run all operations in DB thread.
   */
  class DatabaseService : public base::RefCountedThreadSafe<DatabaseService> {
   public:
    explicit DatabaseService(const base::FilePath& db_path);

    /**
     * Load data from database.
     * Pollutes @p root with fetched data.
     */
    bool Load(const scoped_refptr<RootFolderHolder>& root);
    bool UpgradeToVersion3();

    /**
     * Add or update favorite data in database.
     */
    void SaveFavoriteData(const scoped_refptr<FavoriteRawDataHolder>& data);
    void SafeSaveFavoriteDataOnExit
        (const scoped_refptr<FavoriteRawDataHolder>& data);

    /**
     * Clear database
     */
    void Clear();

    /**
     * Fetch and recreate Faorites selected by @p query.
     */
    bool LoadSelected(const char* query,
                      const scoped_refptr<RootFolderHolder>& root);

    /**
     * Cancel pending save.
     */
    void Stop();

    /**
     * Is stopped?
     */
    bool IsStopped() const;

    /**
     * Make sure data is written to disk
     */
    void Flush();

   private:
    friend class base::RefCountedThreadSafe<DatabaseService>;
    typedef std::vector<std::pair<std::string, std::string> > GuidToDataMapping;

    ~DatabaseService();

    /**
     * Should continue saving?
     */
    bool ContinueSave() const;

    /**
     * Extract a mapping from guid to serialized data blob from the database.
     */
    void GetFavoriteData(GuidToDataMapping* data);

    /**
     * Update the serialized data blob in the database to the given mapping.
     */
    bool WriteFavoriteData(const GuidToDataMapping& data);

    /**
     * Insert a list of all empty folders on the root level into the guids
     * vector.
     */
    void GetEmptyFoldersGuid(std::vector<std::string>* guids);

    /**
     * @param guid The guid of the folder to check.
     * @param nr_of_children The number of children will be written to
     *                       nr_of_children.
     * @return True if the database operations succeded.
     */
    bool GetFoldersChildCount(const std::string& guid, int* nr_of_children);

    /**
     * Calling this method will upgrade the serialized data blob to be version
     * numbered.
     *
     * @return True if the database operations succeded.
     */
    bool UpgradeFavoriteDataFormat();

    /**
     * Deletes all empty folders in the database.
     *
     * @return True if the database operations succeded.
     */
    bool DeleteEmptyFolders();

    /**
     * Delete the favorite identified by guid.
     *
     * @return True if the database operations succeded.
     */
    bool DeleteFavorite(const std::string& guid);

    /**
     * DB file name with full path.
     */
    base::FilePath db_path_;

    /**
     * Favorites database connection.
     */
    sql::Connection db_;

    /**
     * Cancellation flag used to cancel pending save.
     */
    base::CancellationFlag stopped_;

    /**
     * Save despite stopped_ flag?
     */
    bool force_save_if_stopped_;
  };

  /**
   * Current database version.
   */
  static const int kCurrentVersionNumber = 3;

  /*
   * Minimum supported database version.
   */
  static const int kCompatibleVersionNumber = 1;

  virtual ~FavoriteDBStorage();

  /**
   * Release storage.
   */
  void Release();

  /**
   * Init loading.
   * Posts DB initialization and loading task to DB thread.
   */
  void InitLoading(const LoadedCallback& callback);

  /**
   * Called after loading data from database has been finished.
   */
  void OnDataLoaded(const LoadedCallback& callback,
                    const scoped_refptr<RootFolderHolder>& root,
                    bool result);

  /**
   * Called after saving data to database.
   */
  void OnDataSaved(const SavedCallback& callback);

  /**
   * Path to folder in which database files should reside
   */
  base::FilePath db_path_;

  /**
   * Task runner for the thread on which database operations should be
   * performed.
   */
  scoped_refptr<base::SingleThreadTaskRunner> db_task_runner_;

  /**
   * Is database ready to be used?
   */
  bool ready_to_use_;

  /**
   * Database service.
   */
  scoped_refptr<DatabaseService> db_service_;
};

}  // namespace opera

#endif  // COMMON_FAVORITES_FAVORITE_DB_STORAGE_H_
