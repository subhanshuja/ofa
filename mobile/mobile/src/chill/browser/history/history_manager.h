// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_HISTORY_HISTORY_MANAGER_H_
#define COMMON_HISTORY_HISTORY_MANAGER_H_

#include <set>
#include <string>
#include <map>

#include "base/task/cancelable_task_tracker.h"
#include "components/history/core/browser/history_service_observer.h"
#include "components/history/core/browser/history_types.h"
#include "components/history/core/browser/url_row.h"

#include "common/swig_utils/op_runnable.h"
#include "common/swig_utils/op_arguments.h"

class Profile;

namespace history {
class HistoryService;
}

namespace opera {

// Responsible for observing changes to the history and notifying
// the assigned callback with all the changes.
class HistoryManager : public history::HistoryServiceObserver {
 public:
  HistoryManager();
  ~HistoryManager() override;

  // Set the callback to be called when history has been changed.
  // The callback will be sent with with a list of history entries.
  void SetCallback(OpRunnable callback);

  // Enabled/disabled whether we should listen for history changes.
  void Observe(bool enabled);

  // Perform a force update, will notify the callback with all changes
  // to the history.
  void ForceUpdate();

  // Clear history and run callback
  void Clear(OpRunnable callback);

  // Remove the url from history.
  void Remove(GURL url);

  // Returns the time when given URL was accessed from the first time via
  // OpRunnable
  void GetFirstVisitTime(GURL url, OpRunnable callback);

  // history::HistoryServiceObserver
  void OnURLVisited(history::HistoryService* history_service,
                    ui::PageTransition transition,
                    const history::URLRow& row,
                    const history::RedirectList& redirects,
                    base::Time visit_time) override;
  void OnURLsModified(history::HistoryService* history_service,
                      const history::URLRows& changed_urls) override;
  void OnHistoryServiceLoaded(
      history::HistoryService* history_service) override;
  void OnURLsDeleted(history::HistoryService* history_service,
                     bool all_history,
                     bool expired,
                     const history::URLRows& deleted_rows,
                     const std::set<GURL>& favicon_urls) override;

 private:
  void Update();
  void OnHistoryChanged();
  void OnHistoryDeletionDone(OpRunnable callback);
  void OnGetFirstVisitTimeDone(OpRunnable callback,
                               bool success,
                               int visitsCount,
                               base::Time firstVisitTime);
  // Callback method for when HistoryService query results are ready with the
  // most recently-visited sites.
  void OnVisitedHistoryResults(history::QueryResults* results);

  history::HistoryService* GetHistoryService();

  Profile* profile_;
  OpRunnable callback_;
  bool need_recreate_;
  bool create_in_progress_;
  bool enabled_;
  bool waiting_for_clear_history_;
  base::CancelableTaskTracker cancelable_task_tracker_;
};

class OpGetFirstVisitTimeArguments : public OpArguments {
 public:
  bool success;
  // Can be used only if `success == true`
  int visits_count;
  // Can be used only if `success == true`
  base::Time first_visit_time;

 private:
  SWIG_CLASS_NAME
};

}  // namespace opera

#endif  // COMMON_HISTORY_HISTORY_MANAGER_H_
