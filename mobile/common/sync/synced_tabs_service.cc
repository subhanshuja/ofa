// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/sync/synced_tabs_service.h"

#include "base/bind.h"
#include "base/logging.h"

#include "mobile/common/sync/synced_tabs_database.h"

namespace mobile {

namespace {

static const char* kThreadName = "Mobile_SyncedTabsThread";
static const double kUpdateDelaySec = 5;

}  // namespace

SyncedTabsService::SyncedTabsService(const base::FilePath& db_name)
    : thread_(new base::Thread(kThreadName)),
      db_(SyncedTabsDatabase::create()),
      cache_not_loaded_(&cache_lock_),
      cache_loaded_(false) {

  base::Thread::Options options;
  options.timer_slack = base::TIMER_SLACK_MAXIMUM;
  thread_->StartWithOptions(options);

  ScheduleTask(base::Bind(base::IgnoreResult(
      base::Bind(&SyncedTabsDatabase::Init, db_.get(), db_name))));

  ScheduleTask(base::Bind(&SyncedTabsService::DoLoad, base::Unretained(this),
                          db_));
}

SyncedTabsService::~SyncedTabsService() {
  WaitForCacheToLoad();
  Flush();

  db_ = nullptr;
  base::Thread* thread = thread_;
  thread_ = nullptr;
  delete thread;
}

void SyncedTabsService::Flush() {
  if (timer_.IsRunning()) {
    ForceUpdate();
  }
}

int SyncedTabsService::Lookup(SyncedTabData::id_type id) {
  WaitForCacheToLoad();
  auto it = cache_.find(id);
  return it == cache_.end() ? SyncedTabsDatabase::kInvalidSyncId : it->second;
}

void SyncedTabsService::Insert(SyncedTabData::id_type id, int syncId) {
  if (syncId == SyncedTabsDatabase::kInvalidSyncId) {
    DCHECK(false);
    return;
  }
  WaitForCacheToLoad();
  auto ret = cache_.insert(std::make_pair(id, syncId));
  Entry entry;
  entry.id = id;
  entry.syncId = syncId;
  if (ret.second) {
    entry.insert = true;
  } else {
    DCHECK(false);  // An insert should only be an insert
    if (ret.first->second == syncId) {
      // No new value inserted
      return;
    }
    ret.first->second = syncId;
    entry.insert = false;
  }
  queue_.push_back(entry);
  ScheduleUpdate();
}

void SyncedTabsService::Update(SyncedTabData::id_type id, int syncId) {
  if (syncId == SyncedTabsDatabase::kInvalidSyncId) {
    DCHECK(false);
    return;
  }
  WaitForCacheToLoad();
  auto it = cache_.find(id);
  Entry entry;
  entry.id = id;
  entry.syncId = syncId;
  if (it != cache_.end()) {
    if (it->second == syncId) {
      return;
    }
    it->second = syncId;
    entry.insert = false;
  } else {
    DCHECK(false);  // An update should only be an update
    cache_.insert(std::make_pair(id, syncId));
    entry.insert = true;
  }
  queue_.push_back(entry);
  ScheduleUpdate();
}

void SyncedTabsService::Delete(SyncedTabData::id_type id) {
  WaitForCacheToLoad();
  auto it = cache_.find(id);
  if (it == cache_.end())
    return;
  cache_.erase(it);
  Entry entry;
  entry.id = id;
  entry.syncId = SyncedTabsDatabase::kInvalidSyncId;
  entry.insert = false;
  queue_.push_back(entry);
  ScheduleUpdate();
}

void SyncedTabsService::ScheduleUpdate() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!timer_.IsRunning()) {
    timer_.Start(FROM_HERE, base::TimeDelta::FromSeconds(kUpdateDelaySec), this,
                 &SyncedTabsService::DoScheduledUpdate);
  }
}

void SyncedTabsService::ForceUpdate() {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (timer_.IsRunning()) {
    timer_.Stop();
  }

  DoScheduledUpdate();
}

void SyncedTabsService::DoScheduledUpdate() {
  DCHECK(thread_checker_.CalledOnValidThread());

  std::vector<Entry> queue;
  queue.swap(queue_);
  ScheduleTask(base::Bind(&SyncedTabsService::DoUpdate, db_, queue));
}

void SyncedTabsService::ScheduleTask(const base::Closure& task) {
  DCHECK(thread_checker_.CalledOnValidThread());
  CHECK(thread_);
  CHECK(thread_->message_loop());
  thread_->task_runner()->PostTask(FROM_HERE, task);
}

void SyncedTabsService::DoLoad(const scoped_refptr<SyncedTabsDatabase> db) {
  // Note that this runs on the database thread
  std::map<SyncedTabData::id_type, int> entries;
  db->Load(&entries);

  base::AutoLock lock(cache_lock_);
  cache_.swap(entries);
  cache_loaded_ = true;
  cache_not_loaded_.Broadcast();
}

// static
void SyncedTabsService::DoUpdate(const scoped_refptr<SyncedTabsDatabase> db,
                                 std::vector<Entry> queue) {
  // Note that this runs on the database thread
  for (auto& entry : queue) {
    if (entry.syncId == SyncedTabsDatabase::kInvalidSyncId) {
      db->Delete(entry.id);
    } else if (entry.insert) {
      db->Insert(entry.id, entry.syncId);
    } else {
      db->Update(entry.id, entry.syncId);
    }
  }
}

void SyncedTabsService::WaitForCacheToLoad() {
  DCHECK(thread_checker_.CalledOnValidThread());

  base::AutoLock lock(cache_lock_);
  while (!cache_loaded_) {
    cache_not_loaded_.Wait();
  }
}

}  // namespace mobile

