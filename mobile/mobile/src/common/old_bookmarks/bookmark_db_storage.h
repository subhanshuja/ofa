// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_DB_STORAGE_H_
#define MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_DB_STORAGE_H_

#include <string>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/cancellation_flag.h"
#include "sql/connection.h"

#include "common/old_bookmarks/bookmark.h"
#include "common/old_bookmarks/bookmark_storage.h"

namespace opera {

/**
 * Bookmarks persistent storage backend implemented with usage of SQLite .
 */
class BookmarkDBStorage
  : public BookmarkStorage,
    public base::SupportsWeakPtr<BookmarkDBStorage> {
 public:
  BookmarkDBStorage(base::FilePath db_dir,
                    scoped_refptr<base::SingleThreadTaskRunner> db_task_runner);

  /**
   * Load the bookmarks from persistent storage.
   * This method will create new BookmarkRootFolder and populate it with
   * data fetched from persistent storage. Once loaded callback is called,
   * it should take ownership of new_root passed to it.
   */
  void Load(const LoadedCallback& loaded) override;

    /**
   * Add or update bookmark data in persistent storage.
   */
  virtual void SaveBookmarkData(
      const scoped_refptr<BookmarkRawDataHolder>& data,
      const SavedCallback& callback) override;

  virtual void SafeSaveBookmarkDataOnExit(
      const scoped_refptr<BookmarkRawDataHolder>& data) override;

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
   * BookmarkRootFolder. It is used in order to safely pass pointer
   * to BookmarkRootFolder between threads.
   */
  class RootFolderHolder
    : public base::RefCountedThreadSafe<RootFolderHolder> {
   public:
    RootFolderHolder();

    std::unique_ptr<BookmarkRootFolder> root_folder_;

   private:
    ~RootFolderHolder();
    friend class base::RefCountedThreadSafe<RootFolderHolder>;
  };

  /**
   * DatabaseService class.
   * Implement DB operations on bookmarks DB. Run all operations in DB thread.
   */
  class DatabaseService : public base::RefCountedThreadSafe<DatabaseService> {
   public:
    explicit DatabaseService(const base::FilePath& db_path);

    /**
     * Load data from database.
     * Pollutes @p root with fetched data.
     */
    bool Load(const scoped_refptr<RootFolderHolder>& root);

    /**
     * Add or update bookmark data in database.
     */
    void SaveBookmarkData(const scoped_refptr<BookmarkRawDataHolder>& data);
    void SafeSaveBookmarkDataOnExit
        (const scoped_refptr<BookmarkRawDataHolder>& data);

    /**
     * Clear database
     */
    void Clear();

    /**
     * Fetch and recreate bookmark data from database.
     */
    bool LoadBookmarkData(const scoped_refptr<RootFolderHolder>& root);

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

    ~DatabaseService();

    /**
     * Should continue saving?
    */
    bool ContinueSave() const;

    /**
     * DB file name with full path.
     */
    base::FilePath db_path_;

    /**
     * Bookmarks database connection.
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
  static const int kCurrentVersionNumber = 1;

  /*
   * Minimum supported databse version.
   */
  static const int kCompatibleVersionNumber = 1;

  virtual ~BookmarkDBStorage();

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

  base::FilePath db_dir_;

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

extern const char* kBookmarkDatabaseFilename;

}  // namespace opera

#endif  // MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_DB_STORAGE_H_
