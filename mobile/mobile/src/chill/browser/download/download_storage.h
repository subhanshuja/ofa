// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_DOWNLOAD_DOWNLOAD_STORAGE_H_
#define CHILL_BROWSER_DOWNLOAD_DOWNLOAD_STORAGE_H_

#include <map>
#include <list>
#include <string>
#include <utility>

#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"
#include "base/timer/timer.h"
#include "components/prefs/json_pref_store.h"
#include "content/public/browser/download_item.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/download_manager_delegate.h"

namespace base {
class DictionaryValue;
class ListValue;
}  // namespace base

namespace opera {

class DownloadReadFromDiskObserver;

class DownloadStorage : public content::DownloadItem::Observer,
    public content::DownloadManager::Observer, public JsonPrefStore::Observer,
    public JsonPrefStore::ReadErrorDelegate {
 public:
  DownloadStorage();
  ~DownloadStorage();

  void SetDownloadManager(content::DownloadManager* manager) {
    manager_ = manager;
    manager_->AddObserver(this);
  }

  void GetNextId(const content::DownloadIdCallback& callback);

  void Load(DownloadReadFromDiskObserver* observer);
  void Save();

  void MigrateDownload(std::string filename,
                       std::string url,
                       std::string mimetype,
                       int downloaded,
                       int total,
                       bool completed);

  // content::DownloadManager::Observer overrides:
  virtual void OnDownloadCreated(
      content::DownloadManager* manager,
      content::DownloadItem* item) override;
  void ManagerGoingDown(content::DownloadManager* manager) override;

  // content::DownloadItem::Observer overrides:
  void OnDownloadUpdated(content::DownloadItem* download) override;
  void OnDownloadRemoved(content::DownloadItem* download) override;

  // JsonPrefStore::Observer (don't need to observe values):
  void OnPrefValueChanged(const std::string& key) override {}
  void OnInitializationCompleted(bool succeeded) override;

  // JsonPrefStore::ReadErrorDelegate overrides (Ignore invalid file):
  void OnError(JsonPrefStore::PrefReadError error) override {
    NotifyReadFromDisk();
  }

 private:
  struct MigrateDownloadArgs {
    std::string filename;
    std::string url;
    std::string mimetype;
    int downloaded;
    int total;
    bool completed;
  };
  void MigrateDownloadWithId(MigrateDownloadArgs args, uint32_t id);

  void LoadFileSizes();

  /**
   * Create downloads in batches.
   *
   * Starting conditions:
   * @param offset 0
   * @param max_id kInvalidId
   */
  void CreateDownloads(int offset, uint32_t max_id);

  // Returns false if creation was postponed until download ids can ge generated
  bool CreateDownload(base::DictionaryValue *info, uint32_t *id);
  void CreateDownloadWithId(base::DictionaryValue *info, uint32_t id);

  void NotifyReadFromDisk();
  void ScheduleSave();

  content::DownloadManager* manager_;

  scoped_refptr<JsonPrefStore> store_;
  int store_version_;

  uint32_t next_download_id_;
  std::list<content::DownloadIdCallback> deferred_id_callbacks_;
  std::map<int, base::DictionaryValue *> items_;
  const base::ListValue *items_loaded_;
  base::Lock items_lock_;
  bool loaded_;
  DownloadReadFromDiskObserver* observer_;

  base::OneShotTimer timer_;
};

}  // namespace opera

#endif  // CHILL_BROWSER_DOWNLOAD_DOWNLOAD_STORAGE_H_
