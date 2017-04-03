// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/download/download_storage.h"

#include <algorithm>
#include <iterator>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/synchronization/lock.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/prefs/pref_filter.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_item.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/download_interrupt_reasons.h"
#include "content/public/browser/download_danger_type.h"
#include "url/gurl.h"

#include "chill/browser/download/download_helper.h"

#define STORE_VERSION 2

namespace opera {

static const uint32_t kInvalidId = 0;

namespace {
const char* kId = "id";
const char* kPath = "path";
const char* kUrl = "url";
const char* kReferrer = "referrer";
const char* kMime = "mime";
const char* kETag = "etag";
const char* kLastModified = "last_modified";
const char* kStart = "start";
const char* kEnd = "end";
const char* kTotal = "total";
const char* kState = "state";
const char* kPaused = "paused";
const char* kDownloads = "downloads";
const char* kVersion = "version";
const char* kReceived = "received";
const char* kDisplayName = "display_name";
}  // namespace

using base::FilePath;
using content::DownloadItem;

const int kDefaultScheduleSaveDownloadsIntervalMs = 200;

DownloadStorage::DownloadStorage()
    : manager_(NULL),
      store_version_(0),
      next_download_id_(kInvalidId),
      items_loaded_(NULL),
      items_lock_(),
      loaded_(false),
      observer_(NULL) {
  FilePath pref_filename;
  if (PathService::Get(base::DIR_ANDROID_APP_DATA, &pref_filename)) {
    pref_filename = pref_filename.Append(FILE_PATH_LITERAL(kDownloads));

    scoped_refptr<base::SequencedTaskRunner> sequenced_task_runner =
        JsonPrefStore::GetTaskRunnerForFile(
            pref_filename, content::BrowserThread::GetBlockingPool());
    store_ = new JsonPrefStore(
      pref_filename, sequenced_task_runner, std::unique_ptr<PrefFilter>());

    store_->AddObserver(this);

  } else {
    DCHECK(false && "Could not get DIR_ANDROID_APP_DATA path");
  }
}

DownloadStorage::~DownloadStorage() {
  // Remove observers
  if (manager_ != NULL) {
    for (std::map<int, base::DictionaryValue *>::const_iterator i =
        items_.begin(); i != items_.end(); i++) {
      manager_->GetDownload(i->first)->RemoveObserver(this);
    }
    manager_->RemoveObserver(this);
  }
}

void DownloadStorage::ManagerGoingDown(content::DownloadManager* manager) {
  manager_ = NULL;
}

void DownloadStorage::GetNextId(const content::DownloadIdCallback& callback) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  if (next_download_id_ == kInvalidId) {
    deferred_id_callbacks_.push_back(callback);
  } else {
    callback.Run(next_download_id_++);
  }
}

void DownloadStorage::Load(DownloadReadFromDiskObserver* observer) {
  DCHECK(!loaded_);

  loaded_ = true;
  observer_ = observer;
  store_->ReadPrefsAsync(this);
}

void DownloadStorage::MigrateDownload(std::string filename,
                                      std::string url,
                                      std::string mimetype,
                                      int downloaded,
                                      int total,
                                      bool completed) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  MigrateDownloadArgs args;
  args.filename = filename;
  args.url = url;
  args.mimetype = mimetype;
  args.downloaded = downloaded;
  args.total = total;
  args.completed = completed;
  base::Callback<void(uint32_t)> got_id(base::Bind(
      &DownloadStorage::MigrateDownloadWithId, base::Unretained(this), args));
  GetNextId(got_id);
}

void DownloadStorage::MigrateDownloadWithId(MigrateDownloadArgs args,
                                            uint32_t id) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  std::vector<GURL> url_chain;
  url_chain.push_back(GURL(args.url));

  manager_->CreateDownloadItem(
      base::GenerateGUID(),
      id,
      FilePath(args.filename),
      FilePath(args.filename),
      url_chain,
      GURL(""),
      GURL(""),
      GURL(""),
      GURL(""),
      args.mimetype,
      args.mimetype,
      base::Time::Now(),
      args.completed ? base::Time::Now() : base::Time::FromInternalValue(0),
      std::string(),
      std::string(),
      args.downloaded,
      args.total,
      std::string(),
      args.completed ? DownloadItem::COMPLETE : DownloadItem::INTERRUPTED,
      content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
      content::DOWNLOAD_INTERRUPT_REASON_USER_SHUTDOWN,
      false);
}

void DownloadStorage::OnInitializationCompleted(bool succeeded) {
  {
    base::AutoLock lock(items_lock_);

    if (!succeeded) {
      NotifyReadFromDisk();
      return;  // Ignore invalid file
    }

    const base::Value* list_value;
    if (!store_->GetValue(kDownloads, &list_value)) {
      NotifyReadFromDisk();
      return;  // Ignore invalid file
    }

    const base::ListValue* list;
    if (!list_value->GetAsList(&list)) {
      NotifyReadFromDisk();
      return;  // Ignore invalid file
    }

    const base::Value* version_value;
    if (store_->GetValue(kVersion, &version_value)) {
      version_value->GetAsInteger(&store_version_);
    }

    items_loaded_ = list->DeepCopy();
  }

  content::BrowserThread::PostTask(content::BrowserThread::FILE, FROM_HERE,
      base::Bind(&DownloadStorage::LoadFileSizes, base::Unretained(this)));
}

void DownloadStorage::LoadFileSizes() {
  // Load file sizes:
  for (base::ListValue::const_iterator i = items_loaded_->begin();
      i != items_loaded_->end(); i++) {
    base::DictionaryValue *info;
    if ((*i)->GetAsDictionary(&info)) {
      std::string path;
      info->GetString(kPath, &path);
      int64_t received_bytes = 0;
      base::GetFileSize(FilePath(path), &received_bytes);
      info->SetString(kReceived, base::Int64ToString(received_bytes));
    }
  }
  content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
      base::Bind(&DownloadStorage::CreateDownloads,
                 base::Unretained(this), 0, kInvalidId));
}

void DownloadStorage::CreateDownloads(int offset, uint32_t max_id) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  int counter = 0;
  const int kBatchSize = 64;
  base::ListValue::const_iterator i = items_loaded_->begin();
  std::advance(i, offset);

  for (; i != items_loaded_->end(); i++) {
    // Split work into batches to avoid JNI local reference table overflow.
    if (++counter > kBatchSize) {
      content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
          base::Bind(&DownloadStorage::CreateDownloads,
                     base::Unretained(this), offset + kBatchSize, max_id));
      return;
    }
    base::DictionaryValue *info;
    if ((*i)->GetAsDictionary(&info)) {
      uint32_t id;
      if (!CreateDownload(info, &id)) {
        continue;
      }
      if (max_id == kInvalidId || id > max_id) {
        max_id = id;
      }
    }
  }

  if (max_id != kInvalidId) {
    // Base next id on the maximum restored id even though there might be gaps
    // in the restored ids as this is simpler and should be safe as long as the
    // user clears the download list more often than every 4 billion downloads.
    next_download_id_ = max_id + 1;
  }

  NotifyReadFromDisk();
}

void DownloadStorage::NotifyReadFromDisk() {
  // Always run on the UI thread
  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
    content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
        base::Bind(&DownloadStorage::NotifyReadFromDisk,
                   base::Unretained(this)));
    return;
  }
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  // If next_download_id_ is not set then we never restored any downloads.
  if (next_download_id_ == kInvalidId) {
    next_download_id_ = kInvalidId + 1;
  }

  // Call deferred download id requests now that next_download_id_ is known
  for (std::list<content::DownloadIdCallback>::const_iterator callback_it =
      deferred_id_callbacks_.begin();
      callback_it != deferred_id_callbacks_.end(); ++callback_it) {
    GetNextId(*callback_it);
  }
  deferred_id_callbacks_.clear();

  // Don't delete the loaded items before having called the deferred requests
  if (items_loaded_ != NULL) {
    delete items_loaded_;
    items_loaded_ = 0;
  }

  if (observer_) {
    observer_->Ready();
    observer_ = NULL;
  }
}

bool DownloadStorage::CreateDownload(base::DictionaryValue* info,
                                     uint32_t* id) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  std::string temp;

  if (store_version_ >= 2 && info->GetString(kId, &temp)) {
    base::StringToUint(temp, id);
    CreateDownloadWithId(info, *id);
    return true;
  } else {
    GetNextId(base::Bind(&DownloadStorage::CreateDownloadWithId,
                         base::Unretained(this),
                         info));
    return false;
  }
}

void DownloadStorage::CreateDownloadWithId(base::DictionaryValue *info,
                                           uint32_t id) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  std::string temp;

  info->GetString(kPath, &temp);
  FilePath path(temp);
  info->GetString(kUrl, &temp);
  GURL url(temp);
  info->GetString(kReferrer, &temp);
  GURL referrer_url(temp);

  std::string mime;
  info->GetString(kMime, &mime);

  double time;
  info->GetDouble(kStart, &time);
  base::Time start_time = base::Time::FromDoubleT(time);

  info->GetDouble(kEnd, &time);
  base::Time end_time = base::Time::FromDoubleT(time);

  int64_t received_bytes = 0;
  info->GetString(kReceived, &temp);
  base::StringToInt64(temp, &received_bytes);

  int64_t total_bytes = 0;
  if (store_version_ >= 1) {
    info->GetString(kTotal, &temp);
    base::StringToInt64(temp, &total_bytes);
  } else if (store_version_ == 0) {
    double temp_double = 0;
    info->GetDouble(kTotal, &temp_double);
    total_bytes = temp_double;
  }

  int state_i = 0;
  info->GetInteger(kState, &state_i);
  if (state_i < 0 || state_i > DownloadItem::MAX_DOWNLOAD_STATE) {
    return;  // Illegal state, ignore download
  }
  DownloadItem::DownloadState state =
      static_cast<DownloadItem::DownloadState>(state_i);

  bool paused = false;
  info->GetBoolean(kPaused, &paused);

  if (state != DownloadItem::COMPLETE) {
    state = DownloadItem::INTERRUPTED;
  }

  std::vector<GURL> url_chain;
  url_chain.push_back(url);

  std::string etag, last_modified;
  info->GetString(kETag, &etag);
  info->GetString(kLastModified, &last_modified);

  std::string hash;

  std::string display_name;
  info->GetString(kDisplayName, &display_name);

  DownloadItem* item = manager_->CreateDownloadItem(
      base::GenerateGUID(),
      id,
      path,
      path,
      url_chain,
      referrer_url,
      GURL(""),
      GURL(""),
      GURL(""),
      mime,
      mime,
      start_time,
      end_time,
      etag,
      last_modified,
      received_bytes,
      total_bytes,
      hash,
      state,
      content::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS,
      content::DOWNLOAD_INTERRUPT_REASON_USER_SHUTDOWN,
      false);

  // The display name is overwritten by the raw path name when the new
  // DownloadItem is created and stored to file as a side effect. This ugly hack
  // sets correct display name on the created item and saves it to file again,
  // this time with correct display name.
  if (!display_name.empty()) {
    FilePath name(display_name);
    item->SetDisplayName(name);
    OnDownloadUpdated(item);
  }

  if (state == DownloadItem::INTERRUPTED && !paused) {
    item->Resume();
  }
}

void DownloadStorage::Save() {
  base::AutoLock lock(items_lock_);
  base::ListValue * list = new base::ListValue();

  for (std::map<int, base::DictionaryValue *>::const_iterator i =
      items_.begin(); i != items_.end(); i++) {
    list->Append(i->second->DeepCopy());
  }

  store_->SetValue(kDownloads, base::WrapUnique(list),
                   WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  store_->SetValue(kVersion, base::WrapUnique(new base::FundamentalValue(STORE_VERSION)),
                   WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
  store_->CommitPendingWrite();
}

void DownloadStorage::OnDownloadCreated(content::DownloadManager* manager,
                                        DownloadItem* item) {
  if (item->IsSavePackageDownload()) {
    return;  // Don't add saved pages to the UI
  }

  {
    base::AutoLock lock(items_lock_);
    // Add new item
    items_[item->GetId()] = new base::DictionaryValue();
    item->AddObserver(this);
  }
  // Update item
  OnDownloadUpdated(item);
}

void DownloadStorage::OnDownloadUpdated(DownloadItem *item) {
  {
    base::AutoLock lock(items_lock_);
    base::DictionaryValue *dictionary = items_[item->GetId()];

    dictionary->SetString(kId, base::UintToString(item->GetId()));
    dictionary->SetString(kPath, item->GetFullPath().value());
    dictionary->SetString(kUrl, item->GetURL().spec());
    dictionary->SetString(kReferrer, item->GetReferrerUrl().spec());
    dictionary->SetString(kMime, item->GetMimeType());
    dictionary->SetString(kETag, item->GetETag());
    dictionary->SetString(kLastModified, item->GetLastModifiedTime());
    dictionary->SetDouble(kStart, item->GetStartTime().ToDoubleT());
    dictionary->SetDouble(kEnd, item->GetEndTime().ToDoubleT());
    dictionary->SetString(kTotal, base::Int64ToString(item->GetTotalBytes()));
    dictionary->SetInteger(kState, static_cast<int>(item->GetState()));
    // After restarting opera twice the pause flag of a paused download is
    // incorrect, so the state of the download is an additional condition to
    // set the pause flag (ANDUI-2078)
    DownloadItem::DownloadState state = item->GetState();
    dictionary->SetBoolean(kPaused,
        state != DownloadItem::COMPLETE || item->IsPaused());

    dictionary->SetString(kDisplayName,
        item->GetFileNameToReportUser().value());
  }

  ScheduleSave();
}

void DownloadStorage::OnDownloadRemoved(DownloadItem *item) {
  {
    base::AutoLock lock(items_lock_);
    items_.erase(item->GetId());
  }
  ScheduleSave();
}

void DownloadStorage::ScheduleSave() {
  if (!timer_.IsRunning()) {
    timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(
                     kDefaultScheduleSaveDownloadsIntervalMs), this,
                 &DownloadStorage::Save);
  } else {
    timer_.Reset();
  }
}

}  // namespace opera
