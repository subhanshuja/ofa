// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_FAVORITES_FAVORITE_STORAGE_H_
#define COMMON_FAVORITES_FAVORITE_STORAGE_H_

#include <string>
#include <vector>
#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string16.h"
#include "base/pickle.h"

namespace opera {

class FavoriteRootFolder;
class Favorite;

/**
 * Favorites storage interface.
 */

class FavoriteStorage {
 public:
  typedef base::Callback<void(FavoriteRootFolder* new_root)> LoadedCallback;
  typedef base::Closure SavedCallback;

  /**
    * FavoriteRawDataHolder is a thread safe helper class used for binding
    * favorite data to tasks run on DB thread. It safely makes a copy of
    * all data so all data can be safely transfered to DB thread.
    */
  class FavoriteRawDataHolder
    : public base::RefCountedThreadSafe<FavoriteRawDataHolder> {
   public:
    struct FavoriteRawData {
      FavoriteRawData(const std::string& guid,
                      int index,
                      const base::string16& name,
                      const std::string& url,
                      int type,
                      const std::string& parent_guid,
                      const base::Pickle& serialized_data);
      ~FavoriteRawData();

      std::string guid_;
      int index_;
      base::string16 name_;
      std::string url_;
      int type_;
      std::string parent_guid_;
      base::Pickle serialized_data_;
    };

    FavoriteRawDataHolder();

    void PushFavoriteRawData(const std::string& guid,
                             int index,
                             const base::string16& name,
                             const std::string& url,
                             int type,
                             const std::string& parent_guid,
                             const base::Pickle& serialized_data) {
      data_.push_back(FavoriteRawData(
          guid,
          index,
          name,
          url,
          type,
          parent_guid,
          serialized_data));
    }

    size_t size() const { return data_.size(); }
    const FavoriteRawData& operator[](size_t index) const {
      return data_[index];
    }

   private:
    friend class base::RefCountedThreadSafe<FavoriteRawDataHolder>;

    virtual ~FavoriteRawDataHolder();

    std::vector<FavoriteRawData> data_;
  };

  virtual ~FavoriteStorage() {}

  /**
   * Load the favorites from persistent storage.
   * This method will create new FavoriteRootFolder and pollute it with
   * data fetched from persistent storage. Once loaded callback is called,
   * it should take ownership of new_root passed to it.
   */
  virtual void Load(const LoadedCallback& loaded) = 0;

  /**
   * Add or update favorite data in persistent storage.
   * Favorite is identified by unique @p guid.
   */
  virtual void SaveFavoriteData(
    const scoped_refptr<FavoriteRawDataHolder>& data,
    const SavedCallback& callback) = 0;

  /**
   * Saves favorite data during browser shutdown. Ensures
   * that data is always saved and save itself is not interrupted
   * by shutdown.
   */
  virtual void SafeSaveFavoriteDataOnExit(
      const scoped_refptr<FavoriteRawDataHolder>& data) = 0;

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

#endif  // COMMON_FAVORITES_FAVORITE_STORAGE_H_
