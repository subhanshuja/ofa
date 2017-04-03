// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_STORAGE_H_
#define MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_STORAGE_H_

#include <string>
#include <vector>
#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string16.h"

namespace opera {

class BookmarkRootFolder;
class Bookmark;

/**
 * Bookmarks storage interface.
 */

class BookmarkStorage {
 public:
  typedef base::Callback<void(BookmarkRootFolder* new_root)> LoadedCallback;
  typedef base::Closure SavedCallback;

  /**
    * BookmarkRawDataHolder is a thread safe helper class used for binding
    * bookmark data to tasks run on DB thread. It safely makes a copy of
    * all data so all data can be safely transfered to DB thread.
    */
  class BookmarkRawDataHolder
      : public base::RefCountedThreadSafe<BookmarkRawDataHolder> {
   public:
    struct BookmarkRawData {
      BookmarkRawData(const std::string& guid,
                      int index,
                      const base::string16& name,
                      const std::string& url,
                      int type,
                      const std::string& parent_guid);
      ~BookmarkRawData();

      std::string guid_;
      int index_;
      base::string16 name_;
      std::string url_;
      int type_;
      std::string parent_guid_;
    };

    BookmarkRawDataHolder();

    void PushBookmarkRawData(const std::string& guid,
                             int index,
                             const base::string16& name,
                             const std::string& url,
                             int type,
                             const std::string& parent_guid) {
      data_.push_back(BookmarkRawData(
              guid, index, name, url, type, parent_guid));
    }

    size_t size() const { return data_.size(); }
    const BookmarkRawData& operator[](size_t index) const {
      return data_[index];
    }

   private:
    friend class base::RefCountedThreadSafe<BookmarkRawDataHolder>;

    virtual ~BookmarkRawDataHolder();

    std::vector<BookmarkRawData> data_;
  };

  virtual ~BookmarkStorage() {}

  /**
   * Load the bookmarks from persistent storage.
   * This method will create new BookmarkRootFolder and pollute it with
   * data fetched from persistent storage. Once loaded callback is called,
   * it should take ownership of new_root passed to it.
   */
  virtual void Load(const LoadedCallback& loaded) = 0;

  /**
   * Add or update bookmark data in persistent storage.
   * Bookmark is identified by unique @p guid.
   */
  virtual void SaveBookmarkData(
      const scoped_refptr<BookmarkRawDataHolder>& data,
      const SavedCallback& callback) = 0;

  /**
   * Saves bookmark data during browser shutdown. Ensures
   * that data is always saved and save itself is not interrupted
   * by shutdown.
   */
  virtual void SafeSaveBookmarkDataOnExit(
      const scoped_refptr<BookmarkRawDataHolder>& data) = 0;

  /**
   * Cancel any pending save.
   */
  virtual void Stop() = 0;

  /**
   * Is storage stopped?
   */
  virtual bool IsStopped() const = 0;

  /**
   * Make sure data is written to disk
   */
  virtual void Flush() = 0;
};

}  // namespace opera

#endif  // MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_STORAGE_H_
