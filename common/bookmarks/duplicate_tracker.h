// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_BOOKMARKS_DUPLICATE_TRACKER_H_
#define COMMON_BOOKMARKS_DUPLICATE_TRACKER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/threading/platform_thread.h"
#include "base/timer/timer.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/bookmarks/browser/bookmark_model_observer.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sync/driver/duplicate_tracker_interface.h"
#include "components/sync/driver/sync_service_observer.h"
#include "components/sync_bookmarks/bookmark_model_associator.h"

#include "common/bookmarks/duplicate_sync_helper.h"
#include "common/bookmarks/duplicate_task_runner.h"
#include "common/bookmarks/duplicate_task_runner_observer.h"
#include "common/bookmarks/duplicate_util.h"
#include "common/bookmarks/original_elector.h"
#include "common/bookmarks/tracker_observer.h"
#include "common/sync/sync_observer.h"

class PrefService;

namespace bookmarks {
class BookmarkNode;
}  // namespace bookmarks

namespace opera {

class DuplicateTask;
class DuplicateTrackerDelegate;
class TrackerObserver;

struct StatusDetails {
  StatusDetails();
  ~StatusDetails();
  void Reset();

  int total_flaws;
  int total_duplicates;
  int removed_flaws;
  int removed_duplicates;
  int nodes_seen;
  int ignored_nodes;
};

static const char kOperaDeduplicationBookmarkModelChanged[] =
    "opera.deduplication_bookmark_model_changed";
static const char kOperaDeduplicationLastSuccessfulRun[] =
    "opera.deduplication_last_successful_run";
static const char kOperaDeduplicationBackoffSecods[] =
    "opera-deduplication-backoff-seconds";

// Responsible for scanning the bookmark model, maintaining an map of bookmark
// node hashes, detection of flaws and duplicates. Since the whole
// deduplication process is automatic, clients can register as observers in
// order to fetch progress information only, there is no way to interact with
// the mechanism from outside.
class DuplicateTracker : public bookmarks::BookmarkModelObserver,
                         public TaskRunnerObserver,
                         public syncer::SyncServiceObserver,
                         public opera::SyncObserver,
                         public KeyedService,
                         public DuplicateTrackerInterface {
 public:
  typedef std::vector<BookmarkNodeList> BookmarkDuplicateList;
  typedef std::vector<FlawId> FlawIdList;

  // Represents a single flaw in the bookmark model.
  struct FlawInfo {
    FlawInfo();
    FlawInfo(const FlawInfo&);
    ~FlawInfo();

    // Unique flaw ID.
    FlawId id;
    // The original chosen among all the duplicated nodes for the given flaw.
    const bookmarks::BookmarkNode* original;
    // All the duplicates including the original.
    BookmarkNodeList nodes;
    // Whether the flaw regards a folder or a bookmark.
    bool is_folder;
  };

  enum StatId {
    // Total number of flaws currently present in the model.
    REMAINING_FLAWS,
    // Total number of duplicates currently present in the model.
    REMAINING_DUPLICATES,
    // Flaws removed from the model from the last scan complete state.
    REMOVED_FLAWS,
    // Duplicates removed from the model from the last scan compelete state.
    REMOVED_DUPLICATES,
    // Total number of bookmark nodes that were indexed (both bookmarks and
    // folders). Note that ignored nodes are not counted.
    NODES_SEEN,
    // Total number of ignored parent folders.
    IGNORED_NODES,
    // Total number of (ignored) speeddial folders found.
    SPEEDDIAL_FOLDERS,
    // Total number of nodes in model.
    TOTAL_NODES
  };

  DuplicateTracker(browser_sync::ProfileSyncService* sync_service,
                   bookmarks::BookmarkModel* bookmark_model,
                   PrefService* pref_service);
  ~DuplicateTracker() override;

  void Initialize();

  // KeyedService implementation
  void Shutdown() override;

  void SetDelegate(std::unique_ptr<DuplicateTrackerDelegate> delegate);

  // Starts the tracker mechanism. Note that the tracker is started by the
  // factory already and therefore is started by default.
  void Start();

  // Stops the tracker mechanism, including both indexing the bookmark model
  // and duplicate removal.
  void Stop();

  void SetBookmarkModelAssociator(
      sync_bookmarks::BookmarkModelAssociator* associator) override;

  void AddObserver(TrackerObserver* observer);
  void RemoveObserver(TrackerObserver* observer);

  bookmarks::BookmarkModel* bookmark_model() const { return bookmark_model_; }

  TrackerState state() const { return state_; }
  TrackerSyncState sync_state() const { return sync_state_; }
  ScanSource scan_source() const { return scan_source_; }

  bool IsState(TrackerState state) const;
  bool IsSyncState(TrackerSyncState sync_state) const;
  bool IsScanSource(ScanSource scan_source) const;

  // Next scheduled model scan time, or base::Time() if not scheduled.
  base::Time next_scan_time() const { return next_scan_time_; }
  const std::string next_scan_time_str() const;
  const bookmarks::BookmarkNode* trash_node() const { return trash_node_; }
  const bookmarks::BookmarkNode* speeddial_node() const {
    return speeddial_node_;
  }

  // Returns a list representing internal tracker stats. Stat descriptions
  // and values are mixed in the list one by one, i.e. [D1, V1, D2, V2, ...].
  std::unique_ptr<base::ListValue> GetInternalStats() const override;

  // The full list of flaws currently found in the model. Includes every known
  // flaw.
  FlawIdList GetCurrentIdList();

  // List of duplicates for the given flaw ID.
  const opera::BookmarkNodeList& GetDuplicatesForFlawId(FlawId id);

  // Fetches a signle flaw from the model according the the search criteria
  // given.
  // Note that which specific flaw will be returned upon a call to this method
  // is
  // not defined. Also note, that this method will not iterate over existing
  // flaws,
  // fetching another one requires removing the one currently returned from the
  // model.
  // |search_type| - a bitwise combination of search flags.
  FlawInfo GetFirstFlaw(IdSearchType search_type);

  // Get flaw information basing on the flaw ID.
  FlawInfo GetFlawInfo(FlawId flaw_id);

  // Find a counterpart of the |duplicated_child| in the |original_folder|.
  // What this method does is searching through the bookmark index for a node
  // that is identical to the |duplicated_child| FlawId-wise, with the only
  // difference that it is attached to the |original_parent| rather than
  // to the parent that the |duplicated_child| is attached to.
  // The method will return nullptr in case no such node can be found. A pointer
  // to the found node is returned otherwise.
  const bookmarks::BookmarkNode* FindCounterpart(
      const bookmarks::BookmarkNode* duplicated_child,
      const bookmarks::BookmarkNode* original_parent);

  // Returns true in case the current number of flaws found is 0.
  // Should only be called when tracker state is INDEXING_COMPLETE.
  bool IsModelClean() const;

  // Append the given bookmark |node| the the index. Note that a call to this
  // method will
  // be ignored in case:
  // * the node is a direct child of root (i.e. all top-level folders);
  // * the parent of the node is already in the ignored parents index;
  // * the node is a second-level speeddial root item folder;
  // Note that the node has to exist at the time this method is called.
  // Any delayed call to this method must be cancelled in case the node
  // is removed in the meantime.
  void AppendNodeToMap(const bookmarks::BookmarkNode* node);
  // Remove the given |node| from the index. Same rules apply as for
  // AppendNodeToMap().
  void RemoveNodeFromMap(const bookmarks::BookmarkNode* node);
  // Same as RemoveNodeFromMap() but substituting the parent node with the
  // only explicitly given as |parent|.
  void RemoveNodeFromMapForParent(const bookmarks::BookmarkNode* node,
                                  const bookmarks::BookmarkNode* parent);

  // Calling this method will set a single expected model change, i.e.
  // a change that will not re-trigger a full model indexing when seen.
  // Used during the duplicate removal process to ensure the bookmark
  // model index follows the changes made during removal.
  // Excersise caution when attempting to expect and follow a
  // remove-folder-with-children change, as iteration over all the child nodes
  // may take an unreasonably long time and at the same time once a node with
  // children is deleted, no BookmarkModelObserver remove notifications
  // for the child nodes are trigerred.
  void SetExpectedChange(const BookmarkModelChange& change);

  // Fetch the value of the given stat counter.
  unsigned GetStat(StatId type) const;

  // BookmarkModelObserver implementation
  void BookmarkModelLoaded(bookmarks::BookmarkModel* model,
                           bool ids_reassigned) override;
  void BookmarkNodeMoved(bookmarks::BookmarkModel* model,
                         const bookmarks::BookmarkNode* old_parent,
                         int old_index,
                         const bookmarks::BookmarkNode* new_parent,
                         int new_index) override;
  void BookmarkNodeAdded(bookmarks::BookmarkModel* model,
                         const bookmarks::BookmarkNode* parent,
                         int index) override;
  void BookmarkNodeRemoved(bookmarks::BookmarkModel* model,
                           const bookmarks::BookmarkNode* parent,
                           int old_index,
                           const bookmarks::BookmarkNode* node,
                           const std::set<GURL>& no_longer_bookmarked) override;
  void OnWillRemoveBookmarks(bookmarks::BookmarkModel* model,
                             const bookmarks::BookmarkNode* parent,
                             int old_index,
                             const bookmarks::BookmarkNode* node) override;
  void OnWillChangeBookmarkNode(bookmarks::BookmarkModel* model,
                                const bookmarks::BookmarkNode* node) override;
  void BookmarkNodeChanged(bookmarks::BookmarkModel* model,
                           const bookmarks::BookmarkNode* node) override;
  void BookmarkNodeFaviconChanged(bookmarks::BookmarkModel* model,
                                  const bookmarks::BookmarkNode* node) override;
  void OnWillReorderBookmarkNode(bookmarks::BookmarkModel* model,
                                 const bookmarks::BookmarkNode* node) override;
  void BookmarkNodeChildrenReordered(
      bookmarks::BookmarkModel* model,
      const bookmarks::BookmarkNode* node) override;
  void ExtensiveBookmarkChangesBeginning(
      bookmarks::BookmarkModel* model) override;
  void ExtensiveBookmarkChangesEnded(bookmarks::BookmarkModel* model) override;
  void BookmarkAllUserNodesRemoved(bookmarks::BookmarkModel* model,
                                   const std::set<GURL>& removed_urls) override;

  // TaskRunnerObserver implementation
  void OnQueueEmpty(DuplicateTaskRunner* runner) override;
  void OnTaskFinished(DuplicateTaskRunner* runner,
                      scoped_refptr<DuplicateTask> task) override;

  // sync_driver::SyncServiceObserver implementation
  void OnSyncCycleCompleted() override;
  void OnStateChanged() override;

  // opera::SyncObserver implementation.
  void OnSyncStatusChanged(syncer::SyncService* sync_service) override;

  // Post a deduplication task on the remever's task queue.
  void PostDeduplicationTask(const bookmarks::BookmarkNode* original,
                             const bookmarks::BookmarkNode* duplicate);
  // Post a hash calculation task on the indexer's task queue.
  void PostCalculationTask(const bookmarks::BookmarkNode* node);

  // Append the sync node that corresponds to the given bookmark node
  // to the list of unsynced nodes that are pending synchronization.
  void AddNodeToWaitForSync(const bookmarks::BookmarkNode* node);

  // Check whether there are at most kMaxWaitForSyncNodeCount unsynced
  // nodes pending.
  bool ShouldWaitForSync() const;

  // Determines what should happen to the removed duplicates, whether
  // to drop them from the model or move to trash.
  bool ShouldRemoveDuplicatesFromModel() const;

  // Configure the current default scan delay time.
  // The parameter is a number of milliseconds that will
  // constitute the new default scan delay.
  void SetDefaultScanDelayMs(int default_delay_ms);

  // Set the preference value to zero, stop the timer. Effectively exit
  // backoff.
  void ExitBackoff();

  // Returns true if the tracker is in the backoff period, false otherwise.
  bool IsWithinBackoff() const;

 private:
  enum HistogramType {
    HISTOGRAM_TYPE_INDEXING_FINISHED,
    HISTOGRAM_TYPE_SYNC_CYCLE_COMPLETE,
    HISTOGRAM_TYPE_LAST
  };

  typedef std::map<FlawId, BookmarkNodeList> FlawIdToNodeListMapType;
  typedef std::pair<FlawId, BookmarkNodeList> PairType;
  typedef std::set<FlawId> DuplicateIndexType;
  typedef std::set<int64_t> NodeIdSet;

  const base::TimeDelta kMaxScanDelay = base::TimeDelta::FromMinutes(3);
  const base::TimeDelta kDefaultScanDelay = base::TimeDelta::FromSeconds(5);
  const unsigned kMaxWaitForSyncNodeCount = 20;
  // Avoid too frequent refresh of webui debug (sync-internals).
  const unsigned kDebugStatNotificationStep = 50;
  const base::TimeDelta kDefaultBackoffPeriod = base::TimeDelta::FromDays(1);

  const base::TimeDelta default_scan_delay() const;

  // Do start building the model index.
  void DoStartScan();

  void ChangeState(TrackerState state);
  void ChangeSyncState(TrackerSyncState sync_state);

  FlawId CalculateId(const bookmarks::BookmarkNode* node);
  FlawId CalculateIdForGivenParent(const bookmarks::BookmarkNode* node,
                                   const bookmarks::BookmarkNode* parent);

  // Will be called once we are sure that the bookmark model has been loaded.
  void ProcessBookmarkModelLoaded();

  //  Returns true in case the given |folder| is a direct child of the
  // speeddial root node (i.e. it is a folder node that represents a device).
  bool IsDirectChildOfSpeeddial(const bookmarks::BookmarkNode* folder) const;

  // Returns true in case the given folder node is a descendant of the root
  // speeddial node.
  bool IsChildFolderOfSpeeddial(const bookmarks::BookmarkNode* folder) const;

  // Returns true in case the given bookmark node is a folder that is a
  // non-direct child of the model->speeddial_node() and its name is among
  // ignored folder names (initially "" and "Folder").
  bool IsIgnoredSpeeddialFolder(const bookmarks::BookmarkNode* node);

  // Should a speeddial folder with the given title be ignored.
  bool IsTitleIgnoredForSpeeddial(const base::string16& title) const;

  // Check whether the given bookmark |item| should be ignored. Ignored items
  // include the root item and all child items of already ignored folder
  // items.
  bool ShouldIgnoreItem(const bookmarks::BookmarkNode* item) const;

  // Check whether the given |parent| is an ignored parent.
  bool ShouldIgnoreParent(const bookmarks::BookmarkNode* parent) const;

  // Returns an iterator to the ignored parents' index. Speeds up removal
  // from the index if the |parent| is found to be ignored.
  NodeIdSet::const_iterator FindIgnoredParent(
      const bookmarks::BookmarkNode* parent) const;

  // Process the sync nodes pending synchronization to check which are
  // already synchronized. Unpause the removal queue in case there is
  // room for new model changes due to previous changes becoming synchronized
  // with the remote.
  void CheckAndClearWaitForSync();

  // Set a null expected model change.
  void ResetExpectedChange();

  // Verify is the given model change was set as expected with a
  // SetExpectedChange() call.
  bool IsChangeExpected(const BookmarkModelChange& change);

  // Reset the tracker state to an idle state. Clear both the indexer and
  // remover queues, effectively stopping both processes in case any of them
  // is in progress. Clear all the internal structures and counters. Prepare
  // the tracker to another fresh model scan.
  void ResetTracker();

  // Start a fresh model scan. The |source| argument represents the reason for
  // doing this.
  void RestartScan(ScanSource source);

  void ScheduleCheckAndDeduplicate();

  void CalculateHistograms(HistogramType type);

  // Find a flaw in the model and post a deduplication task in case a flaw
  // is found. Set the tracker state to "indexing complete" in case no flaw
  // can be found. Note that this method will only take flaws that an original
  // can be found for into the account.
  void DoCheckAndDeduplicate();

  // Post a deduplication task for a signle flaw duplicate.
  void StartRemovalOfSingleDuplicate(FlawInfo info);

  void FlawAppearedImpl(FlawId id);
  void FlawDisappearedImpl(FlawId id);
  void DuplicateAppearedImpl(FlawId id);
  void DuplicateDisappearedImpl(FlawId id);

  // Calculate the current scan delay, a value between kDefaultScanDelay and
  // kMaxScanDelay.
  base::TimeDelta CurrentScanDelay(ScanSource source);

  void OnModelChange();

  // Zero all stat counters.
  void ResetStats();

  // Increment/decrement the current value of the given stat counter.
  // Observers will be notified with OnTrackerStatisticsUpdated() in case
  // the current stat value of the given counter is a multipilication of
  // kStatNotificationStep.
  void IncStat(StatId type);
  void DecStat(StatId type);

  // Directly set the current value of the given stat counter.
  // Observers will be notified with OnTrackerStatisticsUpdated()
  // unconditionally.
  void SetStat(StatId type, unsigned value);

  // Counterpart for GetStat(), returns the requested value converted to string.
  std::string GetStatStr(StatId type) const;

  const bookmarks::BookmarkNode* local_speeddial_root();

  // Helper to ensure that the sync service is only observed once.
  void EnsureSyncServiceObserved(bool observed);

  // Check the sync service internal state and update the tracker's
  // sync state accordingly.
  void CheckAndUpdateSyncState();

  // Start the backoff period, set the preference value, start the timer.
  void EnterBackoff();

  // Backoff timer ensures that the tracker resumes operation correctly
  // once the backoff period ends.
  void StartBackoffTimer();
  void StopBackoffTimer();
  void OnBackoffTimerShot();

  // Fetches the last successful run time (full scan/model clean depending
  // on SyncConfig) from prefs. Returns false in case the stored value
  // is zero, returns true otherwise.
  bool GetLastSuccesfulRunTime(base::Time& last_run_time) const;

  // Returns the currently configured backoff period, either the default
  // value or the one configured via commandline for testing.
  base::TimeDelta GetBackoffPeriod() const;

  // Returns the current remaining backoff time. Returns
  // base::TimeDelta::FromSeconds(0) in case the tracker is not in backoff.
  // Use IsWithinBackoff() to determine if the tracker is in backoff.
  base::TimeDelta GetRemainingBackoffDelta() const;

  // Preserve the "model changed" event in preferences. This is used to
  // skip unncesessary scans on browser start-up and backoff end events.
  void SetModelChanged(bool changed);
  bool GetModelChanged() const;

  // The bookmark model has been loaded.
  bool model_loaded_;

  // The bookmark model is performing extensive changes.
  bool extensive_changes_;

  // Maps FlawId values to a BookmarkNodeList, effectively
  // allows fetching all duplicates per given flaw. Note
  // that the map will also contain non-flawed IDs (i.e.
  // the ones for which the bookmark node list has size 1).
  FlawIdToNodeListMapType map_;

  // Set of actually flawed IDs. Updated within the map_ updates,
  // is needed to have the flawed IDs available on request, without
  // the need to iterate through the map_.
  DuplicateIndexType flawed_ids_;

  // Bookmarks node IDs for folders that should be ignored in the
  // flaw scanning.
  NodeIdSet ignored_parents_;

  // The current tracker statistics for sync-internals.
  std::map<StatId, unsigned> stats_;

  // IDs of each and every child folder of the speeddial top-level
  // folder.
  NodeIdSet speeddial_parents_;

  // Holds the sync node IDs for sync nodes that correspond to bookmark
  // model items that were changed during deduplication.
  // The tracker will only allow this set to fill up to
  // kMaxWaitForSyncNodeCount before pausing the deduplication queue.
  // Nodes are removed from the set in case they are found to be synced
  // after a sync cycl.e
  NodeIdSet unsynced_node_ids_;

  // Holds the last scan delay in order to extend it up to kMaxScanDelay
  // if needed.
  base::TimeDelta last_scan_delay_;

  // The current default scan delay, this is the time that the tracker will
  // spend in the "indexing scheduled" state if the scan delay has not been
  // extended due to sync changes.
  base::TimeDelta default_scan_delay_;

  // Next scheduled scan time for sync-internals.
  base::Time next_scan_time_;

  // Set by the remnover in order to prevent rescan on expected
  // model changes.
  BookmarkModelChange expected_change_;

  // The current internal states.
  TrackerState state_;
  TrackerSyncState sync_state_;
  ScanSource scan_source_;

  // Delays the index building as needed.
  base::OneShotTimer indexing_delay_timer_;

  // Ensures tracker takes action after the backoff period ends.
  base::OneShotTimer backoff_timer_;

  bookmarks::BookmarkModel* bookmark_model_;
  browser_sync::ProfileSyncService* sync_service_;
  sync_bookmarks::BookmarkModelAssociator* associator_;
  PrefService* pref_service_;

  const bookmarks::BookmarkNode* speeddial_node_;
  const bookmarks::BookmarkNode* trash_node_;
  const bookmarks::BookmarkNode* local_speeddial_root_;

  std::unique_ptr<OriginalElector> original_elector_;
  std::unique_ptr<DuplicateTaskRunner> index_runner_;
  std::unique_ptr<DuplicateTaskRunner> remove_runner_;
  std::unique_ptr<DuplicateSyncHelper> sync_helper_;

  // We want to describe the number of hierarchy conflicts, the best
  // metric for that appears to be the delta of the value between
  // sync cycles.
  int last_cycle_hierarchy_conflicts_;
  int last_cycle_hierarchy_conflicts_delta_;

  std::unique_ptr<DuplicateTrackerDelegate> delegate_;

  base::ObserverList<TrackerObserver> observers_;

  base::WeakPtrFactory<DuplicateTracker> weak_ptr_factory_;
};
}  // namespace opera
#endif  // COMMON_BOOKMARKS_DUPLICATE_TRACKER_H_
