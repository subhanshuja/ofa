// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/bookmarks/duplicate_tracker.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/browser/opera_bookmark_utils.h"
#include "components/prefs/pref_service.h"
#include "components/sync/engine/cycle/model_neutral_state.h"
#include "net/base/escape.h"

#include "common/bookmarks/calculate_hash_task.h"
#include "common/bookmarks/duplicate_task.h"
#include "common/bookmarks/duplicate_task_runner.h"
#include "common/bookmarks/duplicate_tracker_delegate.h"
#include "common/bookmarks/original_elector.h"
#include "common/bookmarks/original_elector_local.h"
#include "common/bookmarks/original_elector_sync.h"
#include "common/bookmarks/remove_duplicate_task.h"
#include "common/bookmarks/tracker_observer.h"
#include "common/sync/sync_config.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

namespace opera {
StatusDetails::StatusDetails() {
  Reset();
}

StatusDetails::~StatusDetails() {}

void StatusDetails::Reset() {
  total_flaws = 0;
  total_duplicates = 0;
  removed_flaws = 0;
  removed_duplicates = 0;
  nodes_seen = 0;
  ignored_nodes = 0;
}

DuplicateTracker::FlawInfo::FlawInfo() {
  id = "";
  original = nullptr;
  DCHECK(nodes.empty());
  is_folder = false;
}

DuplicateTracker::FlawInfo::FlawInfo(
    const DuplicateTracker::FlawInfo&) = default;

DuplicateTracker::FlawInfo::~FlawInfo() {}

DuplicateTracker::DuplicateTracker(
    browser_sync::ProfileSyncService* sync_service,
    BookmarkModel* bookmark_model,
    PrefService* pref_service)
    : model_loaded_(false),
      extensive_changes_(false),
      last_scan_delay_(default_scan_delay()),
      default_scan_delay_(kDefaultScanDelay),
      expected_change_(BookmarkModelChange::NullChange()),
      state_(TRACKER_STATE_STOPPED),
      sync_state_(SYNC_STATE_DISASSOCIATED),
      scan_source_(SCAN_SOURCE_UNSET),
      bookmark_model_(bookmark_model),
      sync_service_(sync_service),
      associator_(nullptr),
      pref_service_(pref_service),
      speeddial_node_(nullptr),
      trash_node_(nullptr),
      local_speeddial_root_(nullptr),
      last_cycle_hierarchy_conflicts_(0),
      last_cycle_hierarchy_conflicts_delta_(0),
      weak_ptr_factory_(this) {
  DCHECK(bookmark_model_);
  DCHECK(sync_service_);
  DCHECK(pref_service_);

  ResetStats();
}

DuplicateTracker::~DuplicateTracker() {
  indexing_delay_timer_.Stop();
}

void DuplicateTracker::Initialize() {
  index_runner_.reset(new DuplicateTaskRunner(
      DuplicateTaskRunner::QT_FIFO, base::TimeDelta::FromMilliseconds(50)));
  remove_runner_.reset(new DuplicateTaskRunner(
      DuplicateTaskRunner::QT_LIFO, base::TimeDelta::FromMilliseconds(50)));

  DCHECK(index_runner_.get());
  DCHECK(remove_runner_.get());

  index_runner_->SetObserver(this);
  remove_runner_->SetObserver(this);

  original_elector_.reset(new OriginalElectorLocal());

  DCHECK(bookmark_model_);
  bookmark_model_->AddObserver(this);
  sync_service_->AddSyncStateObserver(this);

  CheckAndUpdateSyncState();

  if (bookmark_model_->loaded())
    ProcessBookmarkModelLoaded();
}

void DuplicateTracker::Shutdown() {
  EnsureSyncServiceObserved(false);
  bookmark_model_->RemoveObserver(this);
  sync_service_->RemoveSyncStateObserver(this);

  bookmark_model_ = nullptr;
  sync_service_ = nullptr;
}

void DuplicateTracker::SetDelegate(
    std::unique_ptr<DuplicateTrackerDelegate> delegate) {
  delegate_ = std::move(delegate);
}

void DuplicateTracker::Start() {
  DCHECK(IsState(TRACKER_STATE_STOPPED)) << TrackerStateToString(state());
  VLOG(1) << "Tracker starting";
  ChangeState(TRACKER_STATE_IDLE);
  RestartScan(SCAN_SOURCE_TRACKER_STARTED);
}

void DuplicateTracker::Stop() {
  ResetTracker();
  ChangeState(TRACKER_STATE_STOPPED);
}

void DuplicateTracker::SetBookmarkModelAssociator(
    sync_bookmarks::BookmarkModelAssociator* associator) {
  VLOG(2) << "Associator has been " << (associator ? "set" : "unset");
  associator_ = associator;
}

void DuplicateTracker::AddObserver(TrackerObserver* observer) {
  observers_.AddObserver(observer);
}

void DuplicateTracker::RemoveObserver(TrackerObserver* observer) {
  observers_.RemoveObserver(observer);
}

bool DuplicateTracker::IsState(TrackerState state) const {
  return state_ == state;
}

bool DuplicateTracker::IsSyncState(TrackerSyncState sync_state) const {
  return sync_state_ == sync_state;
}

bool DuplicateTracker::IsScanSource(ScanSource scan_source) const {
  return scan_source_ == scan_source;
}

const std::string DuplicateTracker::next_scan_time_str() const {
  if (next_scan_time().is_null())
    return "Not scheduled";
  return syncer::GetTimeDebugString(next_scan_time());
}

std::unique_ptr<base::ListValue> DuplicateTracker::GetInternalStats() const {
  std::unique_ptr<base::ListValue> list_value(new base::ListValue());

  list_value->Append(base::MakeUnique<base::StringValue>("Tracker state"));
  list_value->Append(
      base::MakeUnique<base::StringValue>(TrackerStateToString(state())));

  list_value->Append(base::MakeUnique<base::StringValue>("Sync state"));
  list_value->Append(base::MakeUnique<base::StringValue>(
      TrackerSyncStateToString(sync_state())));

  list_value->Append(base::MakeUnique<base::StringValue>("Scan source"));
  list_value->Append(
      base::MakeUnique<base::StringValue>(ScanSourceToString(scan_source())));

  list_value->Append(base::MakeUnique<base::StringValue>("Next scan time"));
  list_value->Append(base::MakeUnique<base::StringValue>(next_scan_time_str()));

  std::string desc = "Last full scan time";
  if (opera::SyncConfig::ShouldRemoveDuplicates()) {
    desc = "Last model clean time";
  }
  list_value->Append(base::MakeUnique<base::StringValue>(desc));

  base::Time last_successful_run;
  std::string value = "n/a";
  if (GetLastSuccesfulRunTime(last_successful_run)) {
    value = syncer::GetTimeDebugString(last_successful_run);
  }
  list_value->Append(base::MakeUnique<base::StringValue>(value));

  std::string timer_running = backoff_timer_.IsRunning() ? "[*]" : "[ ]";
  std::string backoff_status = "No";
  if (IsWithinBackoff()) {
    base::Time backoff_end = base::Time::Now() + GetRemainingBackoffDelta();
    backoff_status = "Until " + syncer::GetTimeDebugString(backoff_end);
  }
  list_value->Append(base::MakeUnique<base::StringValue>("Backoff"));
  list_value->Append(base::MakeUnique<base::StringValue>(timer_running + " " +
                                                         backoff_status));

  list_value->Append(base::MakeUnique<base::StringValue>("Model changed"));
  list_value->Append(
      base::MakeUnique<base::StringValue>(GetModelChanged() ? "Yes" : "No"));

  list_value->Append(base::MakeUnique<base::StringValue>("Total nodes"));
  list_value->Append(
      base::MakeUnique<base::StringValue>(GetStatStr(TOTAL_NODES)));

  list_value->Append(base::MakeUnique<base::StringValue>("Nodes seen"));
  list_value->Append(
      base::MakeUnique<base::StringValue>(GetStatStr(NODES_SEEN)));

  list_value->Append(base::MakeUnique<base::StringValue>("Flaw count"));
  list_value->Append(
      base::MakeUnique<base::StringValue>(GetStatStr(REMAINING_FLAWS)));

  list_value->Append(base::MakeUnique<base::StringValue>("Duplicate count"));
  list_value->Append(
      base::MakeUnique<base::StringValue>(GetStatStr(REMAINING_DUPLICATES)));

  list_value->Append(base::MakeUnique<base::StringValue>("Ignored count"));
  list_value->Append(
      base::MakeUnique<base::StringValue>(GetStatStr(IGNORED_NODES)));

  list_value->Append(
      base::MakeUnique<base::StringValue>("Ignored SD folders count"));
  list_value->Append(
      base::MakeUnique<base::StringValue>(GetStatStr(SPEEDDIAL_FOLDERS)));

  list_value->Append(base::MakeUnique<base::StringValue>("Flaws removed"));
  list_value->Append(
      base::MakeUnique<base::StringValue>(GetStatStr(REMOVED_FLAWS)));

  list_value->Append(base::MakeUnique<base::StringValue>("Duplicates removed"));
  list_value->Append(
      base::MakeUnique<base::StringValue>(GetStatStr(REMOVED_DUPLICATES)));

  return list_value;
}

DuplicateTracker::FlawIdList DuplicateTracker::GetCurrentIdList() {
  DCHECK(IsState(TRACKER_STATE_INDEXING_COMPLETE))
      << TrackerStateToString(state());

  FlawIdList list;
  for (const auto& map_it : map_) {
    FlawId id = map_it.first;
    BookmarkNodeList nodes = map_it.second;
    if (nodes.size() > 1)
      list.push_back(id);
  }

  return list;
}

const BookmarkNodeList& DuplicateTracker::GetDuplicatesForFlawId(FlawId id) {
  DCHECK(IsState(TRACKER_STATE_INDEXING_COMPLETE))
      << TrackerStateToString(state());

  FlawIdToNodeListMapType::iterator it = map_.find(id);
  DCHECK(it != map_.end());
  DCHECK_GE(it->second.size(), 2U);
  return it->second;
}

DuplicateTracker::FlawInfo DuplicateTracker::GetFirstFlaw(
    IdSearchType search_type) {
  if (IsSyncState(SYNC_STATE_ASSOCIATING)) {
    VLOG(2) << "Sync state is " << sync_state()
            << ", postponing get first flaw.";
    return FlawInfo();
  }

  DCHECK(original_elector_.get());

  for (const FlawId& flawed_id : flawed_ids_) {
    FlawInfo info = GetFlawInfo(flawed_id);

    if (!info.original && (search_type & HAS_ORIGINAL)) {
      VLOG(3) << "Flaw with no original found";
      continue;
    }

    if (info.is_folder && !(search_type & IS_FOLDER))
      continue;
    if (!info.is_folder && !(search_type & IS_BOOKMARK))
      continue;

    return info;
  }

  return FlawInfo();
}

DuplicateTracker::FlawInfo DuplicateTracker::GetFlawInfo(FlawId flaw_id) {
  FlawInfo info;
  const BookmarkNodeList& nodes = map_[flaw_id];
  DCHECK(nodes.size() > 1);

  const BookmarkNode* first_node = (*(nodes.begin()));
  DCHECK(first_node);
  DCHECK(!ShouldIgnoreItem(first_node));

  const BookmarkNode* original = original_elector_->ElectOriginal(nodes);
  DCHECK(original || IsSyncState(SYNC_STATE_ASSOCIATED));

  info.id = flaw_id;
  info.original = original;
  info.nodes = nodes;
  info.is_folder = first_node->is_folder();
  return info;
}

const BookmarkNode* DuplicateTracker::FindCounterpart(
    const BookmarkNode* duplicated_child,
    const BookmarkNode* original_parent) {
  DCHECK(duplicated_child);
  DCHECK(original_parent);
  DCHECK(original_parent->is_folder());

  DCHECK(IsState(TRACKER_STATE_PROCESSING)) << TrackerStateToString(state());

  FlawId desired_id =
      CalculateIdForGivenParent(duplicated_child, original_parent);
  // In case there are duplicated counterparts in the parent, pick the
  // first one. Deduplication of the duplicated counterparts will be pending.
  FlawIdToNodeListMapType::iterator it = map_.find(desired_id);
  if (it != map_.end()) {
    BookmarkNodeList list = it->second;
    DCHECK(list.size() > 0);
    return *(list.begin());
  }
  return nullptr;
}

bool DuplicateTracker::IsModelClean() const {
  DCHECK(IsState(TRACKER_STATE_INDEXING_COMPLETE) ||
         IsState(TRACKER_STATE_PROCESSING_SCHEDULED))
      << TrackerStateToString(state());
  DCHECK(GetStat(REMAINING_FLAWS) != 0 || GetStat(REMAINING_DUPLICATES) == 0);
  return (GetStat(REMAINING_FLAWS) == 0);
}

void DuplicateTracker::AppendNodeToMap(const BookmarkNode* node) {
  DCHECK(node);
  DCHECK(node->parent());

  if (node->parent()->id() == 0) {
    return;
  }
  auto ignore_it = FindIgnoredParent(node->parent());
  if (ignore_it != ignored_parents_.end()) {
    if (node->is_folder())
      ignored_parents_.insert(node->id());
    VLOG(4) << "Node " << DescribeBookmark(node) << " has ignored parent";
    // Don't add an ignored node to the map.
    return;
  }

  if (node->is_folder() && IsChildFolderOfSpeeddial(node))
    speeddial_parents_.insert(node->id());

  if (IsIgnoredSpeeddialFolder(node)) {
    // Don't index speeddial folders other than to the top-level ones.
    // The top-level speeddial folders may also become duplicated and can be
    // distinguished by their GUID.
    // The sub-level speeddial folders have the names repeated (either empty or
    // "Folder") and that is desired.
    VLOG(4) << "Node " << DescribeBookmark(node) << " is an ignored SD folder.";
    IncStat(SPEEDDIAL_FOLDERS);
    return;
  }

  FlawId id = CalculateId(node);
  BookmarkNodeList& node_list_for_id = map_[id];

  auto insert_it = node_list_for_id.insert(node);
  DCHECK(insert_it.second);

  if (node_list_for_id.size() >= 2) {
    DuplicateAppearedImpl(id);
    if (node_list_for_id.size() == 2)
      FlawAppearedImpl(id);
  }
}

void DuplicateTracker::RemoveNodeFromMap(const BookmarkNode* node) {
  DCHECK(node->parent());
  RemoveNodeFromMapForParent(node, node->parent());
}

void DuplicateTracker::RemoveNodeFromMapForParent(
    const bookmarks::BookmarkNode* node,
    const bookmarks::BookmarkNode* parent) {
  DCHECK(node);
  DCHECK(parent);

  if (parent->id() == 0)
    return;

  // Drop the node from ignored parents index if exists.
  auto ignore_it = FindIgnoredParent(node);
  if (ignore_it != ignored_parents_.end()) {
    ignored_parents_.erase(ignore_it);
  }

  if (ShouldIgnoreItem(parent)) {
    return;
  }

  bool is_ignored_sd_folder = false;
  if (IsIgnoredSpeeddialFolder(node)) {
    // Don't index speeddial folders other than the top-level ones.
    // The top-level speeddial folders may also become duplicated and can be
    // distinguished by their GUID.
    // The sub-level speeddial folders have the names repeated (either empty or
    // "Folder") and that is expected.
    DecStat(SPEEDDIAL_FOLDERS);
    // We need to remove from speeddial_parents_ first.
    is_ignored_sd_folder = true;
  }

  if (node->is_folder()) {
    auto it = speeddial_parents_.find(node->id());
    if (it != speeddial_parents_.end())
      speeddial_parents_.erase(it);
  }

  if (is_ignored_sd_folder)
    return;

  FlawId id = CalculateIdForGivenParent(node, parent);
  FlawIdToNodeListMapType::iterator id_it = map_.find(id);
  if (id_it == map_.end())
    return;

  BookmarkNodeList& node_list_for_id = id_it->second;
  BookmarkNodeList::iterator node_it =
      std::find(node_list_for_id.begin(), node_list_for_id.end(), node);
  if (node_it != node_list_for_id.end()) {
    node_list_for_id.erase(node_it);
  } else {
    NOTREACHED();
    return;
  }

  if (node_list_for_id.size() >= 1) {
    DuplicateDisappearedImpl(id);
    if (node_list_for_id.size() == 1)
      FlawDisappearedImpl(id);
  }
}

void DuplicateTracker::SetExpectedChange(const BookmarkModelChange& change) {
  expected_change_ = change;
}

unsigned DuplicateTracker::GetStat(StatId type) const {
  switch (type) {
    case REMAINING_FLAWS:
      return flawed_ids_.size();
    case IGNORED_NODES:
      return ignored_parents_.size();
    default:
      if (stats_.find(type) == stats_.end())
        return 0;
      return stats_.at(type);
  }
}

std::string DuplicateTracker::GetStatStr(StatId type) const {
  return base::UintToString(GetStat(type));
}

void DuplicateTracker::BookmarkModelLoaded(bookmarks::BookmarkModel* model,
                                           bool ids_reassigned) {
  if (!model_loaded_)
    ProcessBookmarkModelLoaded();
}

void DuplicateTracker::BookmarkNodeMoved(BookmarkModel* model,
                                         const BookmarkNode* old_parent,
                                         int old_index,
                                         const BookmarkNode* new_parent,
                                         int new_index) {
  if (IsChangeExpected(BookmarkModelChange::MoveChange(
          old_parent, old_index, new_parent, new_index))) {
    RemoveNodeFromMapForParent(new_parent->GetChild(new_index), old_parent);
    AppendNodeToMap(new_parent->GetChild(new_index));
    return;
  }

  OnModelChange();
}

void DuplicateTracker::BookmarkNodeAdded(BookmarkModel* model,
                                         const BookmarkNode* parent,
                                         int index) {
  OnModelChange();
}

void DuplicateTracker::BookmarkNodeRemoved(
    BookmarkModel* model,
    const BookmarkNode* parent,
    int old_index,
    const BookmarkNode* node,
    const std::set<GURL>& no_longer_bookmarked) {}

void DuplicateTracker::OnWillRemoveBookmarks(BookmarkModel* model,
                                             const BookmarkNode* parent,
                                             int old_index,
                                             const BookmarkNode* node) {
  if (IsChangeExpected(
          BookmarkModelChange::RemoveChange(parent, old_index, node))) {
    ResetExpectedChange();
    RemoveNodeFromMap(node);
    return;
  }

  OnModelChange();
}

void DuplicateTracker::OnWillChangeBookmarkNode(BookmarkModel* model,
                                                const BookmarkNode* node) {
  OnModelChange();
}

void DuplicateTracker::BookmarkNodeChanged(BookmarkModel* model,
                                           const BookmarkNode* node) {}

void DuplicateTracker::BookmarkNodeFaviconChanged(BookmarkModel* model,
                                                  const BookmarkNode* node) {}

void DuplicateTracker::OnWillReorderBookmarkNode(BookmarkModel* model,
                                                 const BookmarkNode* node) {
  OnModelChange();
}

void DuplicateTracker::BookmarkNodeChildrenReordered(BookmarkModel* model,
                                                     const BookmarkNode* node) {
}

void DuplicateTracker::ExtensiveBookmarkChangesBeginning(
    bookmarks::BookmarkModel* model) {
  VLOG(3) << "Extensive changes begin";
  extensive_changes_ = true;
  ResetTracker();
}

void DuplicateTracker::ExtensiveBookmarkChangesEnded(
    bookmarks::BookmarkModel* model) {
  // Sync turn extensive changes on during association and also during
  // application of remote bookmark model changes.
  VLOG(3) << "Extensive changes end";

  extensive_changes_ = false;
  RestartScan(SCAN_SOURCE_EXTENSIVE_CHANGES_END);
}

void DuplicateTracker::BookmarkAllUserNodesRemoved(
    BookmarkModel* model,
    const std::set<GURL>& removed_urls) {}

void DuplicateTracker::OnQueueEmpty(DuplicateTaskRunner* runner) {
  if (runner == index_runner_.get()) {
    VLOG(3) << "Indexer queue is empty";
    if (IsState(TRACKER_STATE_INDEXING)) {
      VLOG(3) << "Indexing complete!";
      ChangeState(TRACKER_STATE_INDEXING_COMPLETE);
      ScheduleCheckAndDeduplicate();
    }
  } else if (runner == remove_runner_.get()) {
    VLOG(3) << "Remover queue is empty";
    DCHECK(IsState(TRACKER_STATE_PROCESSING)) << TrackerStateToString(state());
    ChangeState(TRACKER_STATE_INDEXING_COMPLETE);
    ScheduleCheckAndDeduplicate();
  } else {
    NOTREACHED();
  }
}

void DuplicateTracker::OnTaskFinished(DuplicateTaskRunner* runner,
                                      scoped_refptr<DuplicateTask> task) {
  if (runner == index_runner_.get()) {
    IncStat(NODES_SEEN);
  } else if (runner == remove_runner_.get()) {
    // Empty.
  } else {
    NOTREACHED();
  }
}

void DuplicateTracker::OnSyncCycleCompleted() {
  if (!IsSyncState(SYNC_STATE_ASSOCIATED)) {
    VLOG(2) << "Ignoring sync cycle since sync state is "
            << TrackerSyncStateToString(sync_state());
    return;
  }

  VLOG(2) << "Sync cycle, state is " << TrackerStateToString(state_)
          << ", sync state is " << TrackerSyncStateToString(sync_state_);

  CheckAndClearWaitForSync();

  syncer::ModelNeutralState state =
      sync_service_->GetLastCycleSnapshot().model_neutral_state();

  last_cycle_hierarchy_conflicts_delta_ =
      state.num_hierarchy_conflicts - last_cycle_hierarchy_conflicts_;
  last_cycle_hierarchy_conflicts_ = state.num_hierarchy_conflicts;
  CalculateHistograms(HISTOGRAM_TYPE_SYNC_CYCLE_COMPLETE);

  if (state.num_successful_bookmark_commits > 0) {
    VLOG(1) << "Sync cycle introduces new bookmark commits.";
    if (IsState(TRACKER_STATE_INDEXING_COMPLETE)) {
      // We might have just gotten new server-known sync node IDs due to a late
      // model change commit, check again for originals.
      ScheduleCheckAndDeduplicate();
    }
  } else {
    VLOG(2) << "No bookmark commits in the sync cycle.";
  }
}

void DuplicateTracker::OnStateChanged() {
  // Empty.
}

void DuplicateTracker::OnSyncStatusChanged(
      syncer::SyncService* sync_service) {
  DCHECK(sync_service);
  DCHECK(sync_service == sync_service_);
  CheckAndUpdateSyncState();
}

void DuplicateTracker::PostDeduplicationTask(const BookmarkNode* original,
                                             const BookmarkNode* duplicate) {
  DCHECK(opera::SyncConfig::ShouldRemoveDuplicates());
  DCHECK(original && duplicate);
  DCHECK((original->is_url() && duplicate->is_url()) ||
         (original->is_folder() && duplicate->is_folder()));

  DuplicateTask* task = new RemoveDuplicateTask(this, original, duplicate);
  DCHECK(task);
  remove_runner_->PostTask(task);
}

void DuplicateTracker::PostCalculationTask(
    const bookmarks::BookmarkNode* node) {
  index_runner_->PostTask(new CalculateHashTask(this, node));
}

void DuplicateTracker::AddNodeToWaitForSync(
    const bookmarks::BookmarkNode* node) {
  DCHECK(IsSyncState(SYNC_STATE_ASSOCIATED))
      << TrackerSyncStateToString(sync_state());
  DCHECK(sync_helper_.get());

  int64_t sync_id = sync_helper_->GetSyncIdForBookmarkId(node->id());
  DCHECK_EQ(sync_helper_->GetNodeSyncState(sync_id), NodeSyncState::SYNCED);
  auto it = unsynced_node_ids_.insert(sync_id);
  // Assert that we didn't insert this node before.
  DCHECK(it.second);

  if (ShouldWaitForSync()) {
    if (IsState(TRACKER_STATE_PROCESSING)) {
      ChangeState(TRACKER_STATE_WAITING_FOR_SYNC);
      remove_runner_->Pause();
    }
  }
}

bool DuplicateTracker::ShouldWaitForSync() const {
  return (unsynced_node_ids_.size() >= kMaxWaitForSyncNodeCount);
}

bool DuplicateTracker::ShouldRemoveDuplicatesFromModel() const {
  return true;
}

void DuplicateTracker::SetDefaultScanDelayMs(int default_delay_ms) {
  default_scan_delay_ = base::TimeDelta::FromMilliseconds(default_delay_ms);
}

const base::TimeDelta DuplicateTracker::default_scan_delay() const {
  return default_scan_delay_;
}

void DuplicateTracker::DoStartScan() {
  DCHECK(bookmark_model_);
  DCHECK(model_loaded_);
  DCHECK(index_runner_->IsQueueEmpty());
  DCHECK(map_.empty());
  DCHECK(ignored_parents_.empty());
  DCHECK(flawed_ids_.empty());

  // We might have entered extensive changes in between RestartScan()
  // and the actual call to DoStartScan(). Cancel the scan immediatelly,
  // a new scan will be scheduled once the extensive changes have ended.
  if (extensive_changes_) {
    ChangeState(TRACKER_STATE_IDLE);
    return;
  }

  next_scan_time_ = base::Time();
  ChangeState(TRACKER_STATE_INDEXING);

  DCHECK(bookmark_model_->root_node());
  SetStat(TOTAL_NODES, bookmark_model_->root_node()->GetTotalNodeCount());

  ignored_parents_.insert(trash_node_->id());
  speeddial_parents_.insert(speeddial_node_->id());
  for (int i = 0; i < bookmark_model_->root_node()->child_count(); i++) {
    const BookmarkNode* root_child = bookmark_model_->root_node()->GetChild(i);
    // Don't even post tasks for ignored top-level folders.
    if (!ShouldIgnoreParent(root_child)) {
      PostCalculationTask(root_child);
    }
  }
}

void DuplicateTracker::ChangeState(TrackerState state) {
  if (state == state_)
    return;

  if (IsState(TRACKER_STATE_STOPPED)) {
    DCHECK_EQ(state, TRACKER_STATE_IDLE);
  }

  VLOG(1) << "State changed: " << TrackerStateToString(state_) << " -> "
          << TrackerStateToString(state);
  state_ = state;

  switch (state) {
    case TRACKER_STATE_IDLE:
      DCHECK(index_runner_->IsQueueEmpty());
      DCHECK(remove_runner_->IsQueueEmpty());
      break;
    case TRACKER_STATE_INDEXING_SCHEDULED:
      break;
    case TRACKER_STATE_INDEXING:
      break;
    case TRACKER_STATE_INDEXING_COMPLETE:
      last_scan_delay_ = default_scan_delay();
      break;
    case TRACKER_STATE_PROCESSING_SCHEDULED:
      DCHECK(opera::SyncConfig::ShouldRemoveDuplicates());
      break;
    case TRACKER_STATE_PROCESSING:
      DCHECK(opera::SyncConfig::ShouldRemoveDuplicates());
      break;
    case TRACKER_STATE_WAITING_FOR_SYNC:
      DCHECK(opera::SyncConfig::ShouldRemoveDuplicates());
      break;
    default:
      NOTREACHED();
      break;
  }

  FOR_EACH_OBSERVER(TrackerObserver, observers_, OnTrackerStateChanged(this));
  FOR_EACH_OBSERVER(TrackerObserver, observers_, OnDebugStatsUpdated(this));
}

void DuplicateTracker::ChangeSyncState(TrackerSyncState sync_state) {
  if (sync_state_ == sync_state) {
    VLOG(1) << "Ignoring sync state change to "
            << TrackerSyncStateToString(sync_state) << " since this alredy is "
            << "present state";
    return;
  }

  if (sync_state == SYNC_STATE_ASSOCIATED && !associator_) {
    VLOG(1) << "Ignoring sync state change to "
            << TrackerSyncStateToString(sync_state)
            << " since associator is not "
            << "yet set";
    return;
  }

  VLOG(1) << "Sync state changed:  " << TrackerSyncStateToString(sync_state_)
          << " -> " << TrackerSyncStateToString(sync_state);

  sync_state_ = sync_state;

  switch (sync_state) {
    case SYNC_STATE_ASSOCIATED:
      DCHECK(associator_);
      original_elector_.reset(
          new OriginalElectorSync(sync_service_, associator_));
      sync_helper_.reset(new DuplicateSyncHelper(sync_service_, associator_));
      EnsureSyncServiceObserved(true);
      RestartScan(SCAN_SOURCE_SYNC_ENABLED);
      break;
    case SYNC_STATE_ASSOCIATING:
    case SYNC_STATE_ERROR:
      original_elector_.reset();
      sync_helper_.reset();
      EnsureSyncServiceObserved(false);
      ResetTracker();
      break;
    case SYNC_STATE_DISASSOCIATED:
      original_elector_.reset(new OriginalElectorLocal());
      sync_helper_.reset();
      EnsureSyncServiceObserved(false);
      RestartScan(SCAN_SOURCE_SYNC_DISABLED);
      break;
    default:
      NOTREACHED();
      break;
  }
  FOR_EACH_OBSERVER(TrackerObserver, observers_, OnDebugStatsUpdated(this));
}

FlawId DuplicateTracker::CalculateId(const BookmarkNode* node) {
  DCHECK(node);
  DCHECK(node->parent());
  return CalculateIdForGivenParent(node, node->parent());
}

FlawId DuplicateTracker::CalculateIdForGivenParent(const BookmarkNode* node,
                                                   const BookmarkNode* parent) {
  DCHECK(node);
  DCHECK(node->parent());

  FlawId id;
  int64_t parent_id = parent->id();
  std::string narrow_title = base::UTF16ToUTF8(node->GetTitle());
  narrow_title = net::EscapeForHTML(narrow_title);
  std::string parent_id_string = base::Int64ToString(parent_id);
  std::string is_folder = "T";
  std::string url;
  if (node->is_url()) {
    is_folder = "F";
    url = node->url().spec();
    url = net::EscapeForHTML(url);
  }
  id = narrow_title + "&" + url + "&" + parent_id_string + is_folder;
  // Append the GUID only when apppropriate. Saves us some metainfo
  // key search in case we don't expect to find the GUID anyway.
  if (parent == speeddial_node_) {
    std::string node_guid;
    node->GetMetaInfo(opera::kBookmarkSpeedDialRootFolderGUIDKey, &node_guid);
    id = node_guid + id;
  }
  return id;
}

void DuplicateTracker::ProcessBookmarkModelLoaded() {
  DCHECK(bookmark_model_);
  DCHECK(bookmark_model_->loaded());
  model_loaded_ = true;
  speeddial_node_ = OperaBookmarkUtils::GetSpeedDialNode(bookmark_model_);
  trash_node_ = OperaBookmarkUtils::GetTrashNode(bookmark_model_);
  DCHECK(speeddial_node_ && trash_node_);
  extensive_changes_ = bookmark_model_->IsDoingExtensiveChanges();
  RestartScan(SCAN_SOURCE_MODEL_LOADED);
}

bool DuplicateTracker::IsDirectChildOfSpeeddial(
    const bookmarks::BookmarkNode* folder) const {
  DCHECK(folder);
  DCHECK(folder->is_folder());
  return (folder->parent() == speeddial_node_);
}

bool DuplicateTracker::IsChildFolderOfSpeeddial(
    const bookmarks::BookmarkNode* folder) const {
  DCHECK(folder);
  DCHECK(folder->is_folder());
  return (speeddial_parents_.find(folder->parent()->id()) !=
          speeddial_parents_.end());
}

bool DuplicateTracker::IsIgnoredSpeeddialFolder(
    const bookmarks::BookmarkNode* node) {
  DCHECK(node);

  // Not a folder at all.
  if (!node->is_folder())
    return false;

  DCHECK(local_speeddial_root());
  // a second-level speeddial root child that has ignored title
  // or does not belong to the current client.
  if (IsChildFolderOfSpeeddial(node) && !IsDirectChildOfSpeeddial(node)) {
    DCHECK(delegate_);
    if (IsTitleIgnoredForSpeeddial(node->GetTitle()) ||
        !delegate_->IsLocalFavoriteNode(local_speeddial_root(), node))
      return true;
  }

  return false;
}

bool DuplicateTracker::IsTitleIgnoredForSpeeddial(
    const base::string16& title) const {
  if (title.empty())
    return true;

  if (title == delegate_->GetDefaultFolderName())
    return true;

  return false;
}

bool DuplicateTracker::ShouldIgnoreItem(
    const bookmarks::BookmarkNode* item) const {
  DCHECK(item);

  if (item->is_root())
    return true;

  return ShouldIgnoreParent(item->parent());
}

bool DuplicateTracker::ShouldIgnoreParent(const BookmarkNode* parent) const {
  return FindIgnoredParent(parent) != ignored_parents_.end();
}

DuplicateTracker::NodeIdSet::const_iterator DuplicateTracker::FindIgnoredParent(
    const BookmarkNode* parent) const {
  DCHECK(parent);

  return ignored_parents_.find(parent->id());
}

void DuplicateTracker::CheckAndClearWaitForSync() {
  std::vector<int64_t> to_remove;

  for (int64_t sync_id : unsynced_node_ids_) {
    if (sync_helper_->GetNodeSyncState(sync_id) != NodeSyncState::UNSYNCED)
      to_remove.push_back(sync_id);
  }

  for (int64_t id : to_remove)
    unsynced_node_ids_.erase(id);

  VLOG(3) << "Sync nodes became synced: " << to_remove.size();

  if (!ShouldWaitForSync()) {
    if (IsState(TRACKER_STATE_WAITING_FOR_SYNC)) {
      ChangeState(TRACKER_STATE_PROCESSING);
    }
    if (remove_runner_->IsPaused())
      remove_runner_->Unpause();
  }
}

void DuplicateTracker::ResetExpectedChange() {
  expected_change_ = BookmarkModelChange::NullChange();
}

bool DuplicateTracker::IsChangeExpected(const BookmarkModelChange& change) {
  return expected_change_.Equals(change);
}

void DuplicateTracker::ResetStats() {
  stats_.clear();
}

void DuplicateTracker::IncStat(StatId type) {
  stats_[type]++;
  FOR_EACH_OBSERVER(TrackerObserver, observers_,
                    OnTrackerStatisticsUpdated(this));
  if (!(stats_[type] % kDebugStatNotificationStep)) {
    FOR_EACH_OBSERVER(TrackerObserver, observers_, OnDebugStatsUpdated(this));
  }
}

void DuplicateTracker::DecStat(StatId type) {
  stats_[type]--;
  FOR_EACH_OBSERVER(TrackerObserver, observers_,
                    OnTrackerStatisticsUpdated(this));
  if (!(stats_[type] % kDebugStatNotificationStep)) {
    FOR_EACH_OBSERVER(TrackerObserver, observers_, OnDebugStatsUpdated(this));
  }
}

void DuplicateTracker::SetStat(StatId type, unsigned value) {
  stats_[type] = value;
  FOR_EACH_OBSERVER(TrackerObserver, observers_,
                    OnTrackerStatisticsUpdated(this));
  FOR_EACH_OBSERVER(TrackerObserver, observers_, OnDebugStatsUpdated(this));
}

const BookmarkNode* DuplicateTracker::local_speeddial_root() {
  DCHECK(bookmark_model_->loaded());
  if (!local_speeddial_root_) {
    DCHECK(sync_service_ && sync_service_->sync_client());
    PrefService* prefs = sync_service_->sync_client()->GetPrefService();
    DCHECK(delegate_);
    DCHECK(prefs);
    local_speeddial_root_ =
        delegate_->GetLocalFavoritesRootNode(prefs, bookmark_model_);
  }

  DCHECK(local_speeddial_root_);
  return local_speeddial_root_;
}

void DuplicateTracker::EnsureSyncServiceObserved(bool observed) {
  DCHECK(sync_service_);
  if (observed && !sync_service_->HasObserver(this)) {
    sync_service_->AddObserver(this);
  } else if (!observed && sync_service_->HasObserver(this)) {
    sync_service_->RemoveObserver(this);
  }
}

void DuplicateTracker::CheckAndUpdateSyncState() {
  DCHECK(sync_service_);
  SyncStatus status = sync_service_->opera_sync_status();
  syncer::ModelTypeSet types = sync_service_->GetPreferredDataTypes();
  bool has_bookmarks = types.Has(syncer::BOOKMARKS);

  TrackerSyncState state;
  if (status.logged_in()) {
    if (!status.enabled()) {
      state = SYNC_STATE_ERROR;
    } else if (status.is_configuring()) {
      if (has_bookmarks) {
        state = SYNC_STATE_ASSOCIATING;
      } else {
        state = SYNC_STATE_DISASSOCIATED;
      }
    } else {
      if (has_bookmarks) {
        state = SYNC_STATE_ASSOCIATED;
      } else {
        state = SYNC_STATE_DISASSOCIATED;
      }
    }
  } else {
    state = SYNC_STATE_DISASSOCIATED;
  }

  VLOG(2) << status.ToString() << "; has_bookmarks=" << has_bookmarks << " -> "
          << TrackerSyncStateToString(state);
  ChangeSyncState(state);
}

void DuplicateTracker::EnterBackoff() {
  DCHECK(pref_service_);
  DCHECK(!IsWithinBackoff());
  base::Time now = base::Time::Now();
  int64_t now_internal = now.ToInternalValue();
  pref_service_->SetInt64(kOperaDeduplicationLastSuccessfulRun, now_internal);
  DCHECK(IsWithinBackoff());
  VLOG(1) << "Entered backoff. last successful run='"
          << syncer::GetTimeDebugString(now) << "', remaining backoff='"
          << GetRemainingBackoffDelta() << "', backoff='" << GetBackoffPeriod()
          << "'";
  StartBackoffTimer();
  FOR_EACH_OBSERVER(TrackerObserver, observers_, OnDebugStatsUpdated(this));
}

bool DuplicateTracker::IsWithinBackoff() const {
  base::TimeDelta remaining = GetRemainingBackoffDelta();
  if (remaining > base::TimeDelta::FromSeconds(0))
    return true;

  return false;
}

void DuplicateTracker::ExitBackoff() {
  // Stay safe, this method may be used at anytime.
  StopBackoffTimer();
  pref_service_->SetInt64(kOperaDeduplicationLastSuccessfulRun, 0);
  RestartScan(SCAN_SOURCE_BACKOFF_END);
  FOR_EACH_OBSERVER(TrackerObserver, observers_, OnDebugStatsUpdated(this));
}

void DuplicateTracker::StartBackoffTimer() {
  VLOG(4) << "Starting backoff timer.";
  if (!backoff_timer_.IsRunning())
    backoff_timer_.Start(FROM_HERE, GetRemainingBackoffDelta(), this,
                         &DuplicateTracker::OnBackoffTimerShot);
  FOR_EACH_OBSERVER(TrackerObserver, observers_, OnDebugStatsUpdated(this));
}

void DuplicateTracker::StopBackoffTimer() {
  if (backoff_timer_.IsRunning())
    backoff_timer_.Stop();
  FOR_EACH_OBSERVER(TrackerObserver, observers_, OnDebugStatsUpdated(this));
}

void DuplicateTracker::OnBackoffTimerShot() {
  VLOG(2) << "Backoff timer shot.";
  ExitBackoff();
}

bool DuplicateTracker::GetLastSuccesfulRunTime(
    base::Time& last_run_time) const {
  int64_t last_successful_run_internal =
      pref_service_->GetInt64(kOperaDeduplicationLastSuccessfulRun);
  if (last_successful_run_internal == 0) {
    return false;
  }

  last_run_time = base::Time::FromInternalValue(last_successful_run_internal);
  return true;
}

base::TimeDelta DuplicateTracker::GetBackoffPeriod() const {
  base::CommandLine* cl = base::CommandLine::ForCurrentProcess();
  DCHECK(cl);
  if (cl->HasSwitch(kOperaDeduplicationBackoffSecods)) {
    std::string cmdline_backoff =
        cl->GetSwitchValueASCII(kOperaDeduplicationBackoffSecods);
    int cmdline_backoff_int;
    if (base::StringToInt(cmdline_backoff, &cmdline_backoff_int)) {
      return base::TimeDelta::FromSeconds(cmdline_backoff_int);
    } else {
      VLOG(1)
          << "Could not interpret the value for command line backoff period: '"
          << cmdline_backoff << "'. Using default backoff period.";
    }
  }
  return kDefaultBackoffPeriod;
}

base::TimeDelta DuplicateTracker::GetRemainingBackoffDelta() const {
  base::TimeDelta zero_diff = base::TimeDelta::FromSeconds(0);
  base::Time now = base::Time::Now();
  base::Time last_successful_run;
  if (!GetLastSuccesfulRunTime(last_successful_run)) {
    return zero_diff;
  }

  if (now < last_successful_run) {
    VLOG(1) << "Last successful run='"
            << syncer::GetTimeDebugString(last_successful_run)
            << "' is in future (now = '" << syncer::GetTimeDebugString(now)
            << "'), ignoring.";
    pref_service_->SetInt64(kOperaDeduplicationLastSuccessfulRun, 0);
    return zero_diff;
  }

  base::TimeDelta diff = now - last_successful_run;
  base::TimeDelta delta = GetBackoffPeriod() - diff;
  return delta;
}

void DuplicateTracker::SetModelChanged(bool changed) {
  pref_service_->SetBoolean(kOperaDeduplicationBookmarkModelChanged, changed);
  FOR_EACH_OBSERVER(TrackerObserver, observers_, OnDebugStatsUpdated(this));
}

bool DuplicateTracker::GetModelChanged() const {
  return pref_service_->GetBoolean(kOperaDeduplicationBookmarkModelChanged);
}

base::TimeDelta DuplicateTracker::CurrentScanDelay(ScanSource source) {
  if (source == SCAN_SOURCE_EXTENSIVE_CHANGES_END &&
      IsScanSource(SCAN_SOURCE_EXTENSIVE_CHANGES_END) &&
      (IsState(TRACKER_STATE_IDLE) ||
       IsState(TRACKER_STATE_INDEXING_SCHEDULED) ||
       IsState(TRACKER_STATE_INDEXING))) {
    // We want to extend the scan delay in case we continously get remote
    // sync changes to the model.
    // The idea is that the changes may be done by another client performing
    // deduplication and we'd like to keep one client to work on that at a time.
    VLOG(2) << "Extending the scan delay.";
    last_scan_delay_ = std::min(1.5 * last_scan_delay_, kMaxScanDelay);
  } else {
    VLOG(2) << "Scan delay reset to default.";
    last_scan_delay_ = default_scan_delay();
  }

  VLOG(2) << "Current scan delay is " << last_scan_delay_.InSeconds() << " s.";
  next_scan_time_ = base::Time::Now() + last_scan_delay_;
  return last_scan_delay_;
}

void DuplicateTracker::OnModelChange() {
  SetModelChanged(true);
  // Don't react on each model change during extensive changes.
  // A rescan will be trigerred when extensive changes end.
  if (!extensive_changes_)
    RestartScan(SCAN_SOURCE_MODEL_CHANGE);
}

void DuplicateTracker::FlawAppearedImpl(FlawId id) {
  flawed_ids_.insert(id);
  FOR_EACH_OBSERVER(TrackerObserver, observers_, OnFlawAppeared(this, id));

  if (IsState(TRACKER_STATE_INDEXING_COMPLETE) &&
      IsSyncState(SYNC_STATE_DISASSOCIATED))
    ScheduleCheckAndDeduplicate();
}

void DuplicateTracker::FlawDisappearedImpl(FlawId id) {
  flawed_ids_.erase(id);
  IncStat(REMOVED_FLAWS);
  FOR_EACH_OBSERVER(TrackerObserver, observers_, OnFlawDisappeared(this, id));
}

void DuplicateTracker::DuplicateAppearedImpl(FlawId id) {
  IncStat(REMAINING_DUPLICATES);
  FOR_EACH_OBSERVER(TrackerObserver, observers_, OnDuplicateAppeared(this, id));
}

void DuplicateTracker::DuplicateDisappearedImpl(FlawId id) {
  DecStat(REMAINING_DUPLICATES);
  IncStat(REMOVED_DUPLICATES);
  FOR_EACH_OBSERVER(TrackerObserver, observers_,
                    OnDuplicateDisappeared(this, id));
}

void DuplicateTracker::ScheduleCheckAndDeduplicate() {
  DCHECK(IsState(TRACKER_STATE_INDEXING_COMPLETE))
      << TrackerStateToString(state());
  CalculateHistograms(HISTOGRAM_TYPE_INDEXING_FINISHED);

  if (!opera::SyncConfig::ShouldRemoveDuplicates()) {
    VLOG(1) << "Duplicate removal is disabled.";
    SetModelChanged(false);
    EnterBackoff();
    return;
  }

  ChangeState(TRACKER_STATE_PROCESSING_SCHEDULED);

  // Post a call to DoCheckAndDeduplicate() in order to break callstack in
  // case this method is called as a direct result of a sync operation.
  // As DoCheckAndDeduplicate() will attempt to find an original for a
  // found model flaw and the process may include creating a sync
  // transaction in case sync is enabled, we have to ensure that
  // the transaction being the primary reason for this call has
  // already finished.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(&DuplicateTracker::DoCheckAndDeduplicate,
                            weak_ptr_factory_.GetWeakPtr()));
}

void DuplicateTracker::CalculateHistograms(HistogramType type) {
  const int kHierarchyConflictMax = 1000;
  const int kLowNodeCountMax = 10000;
  const int kHighNodeCountMax = 100000;
  const int kBucketCount = 3;
  switch (type) {
    case HISTOGRAM_TYPE_INDEXING_FINISHED:
      DCHECK(IsState(TRACKER_STATE_INDEXING_COMPLETE));
      DCHECK(IsSyncState(SYNC_STATE_ASSOCIATED) ||
             IsSyncState(SYNC_STATE_DISASSOCIATED));
      UMA_HISTOGRAM_CUSTOM_COUNTS(
          "Opera.DSK.BookmarkDuplication.RemainingFlaws",
          GetStat(REMAINING_FLAWS), 1, kLowNodeCountMax, kBucketCount);
      UMA_HISTOGRAM_CUSTOM_COUNTS(
          "Opera.DSK.BookmarkDuplication.RemainingDuplicates",
          GetStat(REMAINING_DUPLICATES), 1, kLowNodeCountMax, kBucketCount);
      // We've seen models over 10k.
      UMA_HISTOGRAM_CUSTOM_COUNTS("Opera.DSK.BookmarkDuplication.NodesSeen",
                                  GetStat(NODES_SEEN), 1, kHighNodeCountMax,
                                  kBucketCount);
      UMA_HISTOGRAM_ENUMERATION("Opera.DSK.BookmarkDuplication.SyncState",
                                sync_state(), SYNC_STATE_LAST);
      break;
    case HISTOGRAM_TYPE_SYNC_CYCLE_COMPLETE:
      if (last_cycle_hierarchy_conflicts_delta_ > 0) {
        UMA_HISTOGRAM_CUSTOM_COUNTS("Opera.DSK.Sync.HierarchyConflict.New",
                                    last_cycle_hierarchy_conflicts_delta_, 1,
                                    kHierarchyConflictMax, kBucketCount);
      } else if (last_cycle_hierarchy_conflicts_delta_ < 0) {
        UMA_HISTOGRAM_CUSTOM_COUNTS("Opera.DSK.Sync.HierarchyConflict.Gone",
                                    -last_cycle_hierarchy_conflicts_delta_, 1,
                                    kHierarchyConflictMax, kBucketCount);
      }
      break;
    default:
      NOTREACHED();
      break;
  }
}

void DuplicateTracker::DoCheckAndDeduplicate() {
  DCHECK(opera::SyncConfig::ShouldRemoveDuplicates());
  DCHECK(index_runner_->IsQueueEmpty());
  DCHECK(remove_runner_->IsQueueEmpty());

  if (IsSyncState(SYNC_STATE_ASSOCIATING)) {
    VLOG(2) << "Postponing flaw processing since sync state is "
            << TrackerSyncStateToString(sync_state());
    return;
  }

  DuplicateTracker::FlawInfo info;
  info = GetFirstFlaw(IdSearchType::HAS_ORIGINAL | IdSearchType::IS_BOOKMARK |
                      IdSearchType::IS_FOLDER);
  if (!info.id.empty()) {
    VLOG(3) << "Found a flaw";
    StartRemovalOfSingleDuplicate(info);
  } else {
    VLOG(1) << "No flaws with originals found, model has "
            << GetStat(REMAINING_FLAWS) << " flaws.";
    if (IsModelClean() && !IsWithinBackoff()) {
      VLOG(1) << "Model is clean, entering backoff.";
      SetModelChanged(false);
      EnterBackoff();
    }
    ChangeState(TRACKER_STATE_INDEXING_COMPLETE);
  }
}

void DuplicateTracker::StartRemovalOfSingleDuplicate(FlawInfo info) {
  DCHECK(opera::SyncConfig::ShouldRemoveDuplicates());
  DCHECK(!info.id.empty());
  DCHECK(info.original);
  DCHECK(info.nodes.size() > 1);

  for (const BookmarkNode* node : info.nodes) {
    if (node == info.original)
      continue;

    ChangeState(TRACKER_STATE_PROCESSING);
    FOR_EACH_OBSERVER(TrackerObserver, observers_,
                      OnFlawProcessingStarted(this, info.id));
    PostDeduplicationTask(info.original, node);
    return;
  }

  NOTREACHED();
}

void DuplicateTracker::RestartScan(ScanSource source) {
  VLOG(1) << "Scan restart requested due to " << ScanSourceToString(source)
          << " last scan requested due to " << ScanSourceToString(scan_source_)
          << " tracker state is " << TrackerStateToString(state_);

  if (IsWithinBackoff()) {
    VLOG(1) << "Scan start request ignored due to backoff period.";
    StartBackoffTimer();
    return;
  }

  if (!model_loaded_) {
    VLOG(1) << "Scan start request ignored due to model not loaded.";
    return;
  }

  if (extensive_changes_) {
    VLOG(1) << "Scan start request ignored due to extensive changes being in "
               "progress.";
    return;
  }

  if (!GetModelChanged()) {
    VLOG(1) << "Ignoring start scan request since the model was not changed.";
    return;
  }

  if (IsState(TRACKER_STATE_STOPPED)) {
    VLOG(1) << "Tracker is stopped, ignoring scan start request";
  } else if (IsSyncState(SYNC_STATE_ASSOCIATED) ||
             IsSyncState(SYNC_STATE_DISASSOCIATED)) {
    base::TimeDelta delay = CurrentScanDelay(source);
    VLOG(1) << "Scan restart due in " << delay.InSeconds() << " s";
    scan_source_ = source;

    ResetTracker();
    indexing_delay_timer_.Start(FROM_HERE, delay, this,
                                &DuplicateTracker::DoStartScan);
    ChangeState(TRACKER_STATE_INDEXING_SCHEDULED);
  } else {
    VLOG(1) << "Scan restart request ignored since sync state is "
            << TrackerSyncStateToString(sync_state_);
  }
}

void DuplicateTracker::ResetTracker() {
  if (IsState(TRACKER_STATE_STOPPED)) {
    VLOG(1) << "Tracker is stopped";
    return;
  }

  index_runner_->ResetQueue();
  remove_runner_->ResetQueue();

  map_.clear();
  ignored_parents_.clear();
  flawed_ids_.clear();
  indexing_delay_timer_.Stop();
  ResetExpectedChange();
  ResetStats();
  ChangeState(TRACKER_STATE_IDLE);
}

}  // namespace opera
