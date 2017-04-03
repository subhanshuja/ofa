// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/history/history_manager.h"

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"

// These are included through mobile/common/sync/sync.gyp.
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/history/history_service_factory.h"

#include "components/history/core/browser/history_service.h"

#include "chill/app/native_interface.h"
#include "chill/browser/history/history_entry.h"
#include "common/sync/sync.h"

namespace {

// Limit the amount of entries received to 100. The entries we get back are
// ordered by the most recent first, so we don't have to worry about getting
// 100 'random' history entries.
static const int MAX_HISTORY_ENTRIES = 100;
// Keep HISTORY_EXPIRE_DAYS_THRESHOLD sync with ExpireDaysThreshold in
// components/history/core/browser/history_backend.cc
static const int HISTORY_EXPIRE_DAYS_THRESHOLD = 90;

opera::HistoryEntry CreateEntry(const history::URLResult& result) {
  opera::HistoryEntry entry;
  entry.id = result.id();
  entry.url = result.url();
  entry.title = base::UTF16ToUTF8(result.title());
  entry.visitTime = result.visit_time();
  return entry;
}

}  // namespace

namespace opera {

HistoryManager::HistoryManager()
    : profile_(mobile::Sync::GetInstance()->GetProfile()),
      need_recreate_(true),
      create_in_progress_(false),
      enabled_(false),
      waiting_for_clear_history_(false) {
  history::HistoryService* hs = GetHistoryService();
  if (hs) {
    hs->AddObserver(this);
  }
}

HistoryManager::~HistoryManager() {
  history::HistoryService* hs = GetHistoryService();
  if (hs) {
    hs->RemoveObserver(this);
  }
}

void HistoryManager::SetCallback(OpRunnable callback) {
  callback_ = callback;
}

void HistoryManager::Observe(bool enabled) {
  if (enabled_ == enabled)
    return;
  enabled_ = enabled;
  OnHistoryChanged();
}

void HistoryManager::ForceUpdate() {
  OnHistoryChanged();
}

void HistoryManager::Clear(OpRunnable callback) {
  history::HistoryService* hs = GetHistoryService();
  if (hs) {
    std::set<GURL> restrict_urls;
    waiting_for_clear_history_ = true;

    hs->ExpireHistoryBetween(restrict_urls, base::Time(), base::Time::Max(),
                             base::Bind(&HistoryManager::OnHistoryDeletionDone,
                                        base::Unretained(this), callback),
                             &cancelable_task_tracker_);
  }
}

void HistoryManager::OnHistoryDeletionDone(OpRunnable callback) {
  waiting_for_clear_history_ = false;
  OnHistoryChanged();
  const OpArguments opargs;
  callback.Run(opargs);
}

void HistoryManager::Remove(GURL url) {
  history::HistoryService* hs = GetHistoryService();
  if (hs) {
    hs->DeleteURL(url);
  }
}

void HistoryManager::GetFirstVisitTime(GURL url, OpRunnable callback) {
  history::HistoryService* hs = GetHistoryService();
  if (!hs) {
    return;
  }

  hs->GetVisibleVisitCountToHost(
      url, base::Bind(&HistoryManager::OnGetFirstVisitTimeDone,
                      base::Unretained(this), callback),
      &cancelable_task_tracker_);
}

void HistoryManager::OnGetFirstVisitTimeDone(OpRunnable callback,
                                             bool success,
                                             int visitsCount,
                                             base::Time firstVisitTime) {
  OpGetFirstVisitTimeArguments args;
  args.success = success;
  if (success) {
    args.visits_count = visitsCount;
    args.first_visit_time = firstVisitTime;
  }
  callback.Run(args);
}

void HistoryManager::Update() {
  // If we're currently running Update(), wait until it finishes.
  if (create_in_progress_ || waiting_for_clear_history_)
    return;
  create_in_progress_ = true;
  need_recreate_ = false;

  history::HistoryService* hs = GetHistoryService();
  if (hs) {
    // Will query the db for all history entries.
    history::QueryOptions options;
    options.max_count = MAX_HISTORY_ENTRIES;
    options.SetRecentDayRange(HISTORY_EXPIRE_DAYS_THRESHOLD);
    hs->QueryHistory(base::string16(), options,
                     base::Bind(&HistoryManager::OnVisitedHistoryResults,
                                base::Unretained(this)),
                     &cancelable_task_tracker_);
  }
}

void HistoryManager::OnHistoryChanged() {
  need_recreate_ = true;
  Update();
}

void HistoryManager::OnVisitedHistoryResults(history::QueryResults* results) {
  size_t count = results->size();

  OpHistoryArguments arg;
  for (size_t i = 0; i < count; ++i) {
    const history::URLResult& result = (*results)[i];
    arg.entries.push_back(CreateEntry(result));
  }

  create_in_progress_ = false;

  if (callback_) {
    callback_.Run(arg);
  }

  if (enabled_ && need_recreate_) {
    Update();
  } else {
    need_recreate_ = false;
  }
}

history::HistoryService* HistoryManager::GetHistoryService() {
  return HistoryServiceFactory::GetForProfile(
      profile_, ServiceAccessType::IMPLICIT_ACCESS);
}

void HistoryManager::OnHistoryServiceLoaded(
    history::HistoryService* history_service) {
  if (enabled_) {
    OnHistoryChanged();
  }
}

void HistoryManager::OnURLVisited(history::HistoryService* history_service,
                                  ui::PageTransition transition,
                                  const history::URLRow& row,
                                  const history::RedirectList& redirects,
                                  base::Time visit_time) {
  if (enabled_) {
    OnHistoryChanged();
  }
}

void HistoryManager::OnURLsModified(history::HistoryService* history_service,
                                    const history::URLRows& changed_urls) {
  if (enabled_) {
    OnHistoryChanged();
  }
}

void HistoryManager::OnURLsDeleted(history::HistoryService* history_service,
                                   bool all_history,
                                   bool expired,
                                   const history::URLRows& deleted_rows,
                                   const std::set<GURL>& favicon_urls) {
  if (enabled_) {
    OnHistoryChanged();
  }
}

}  // namespace opera
