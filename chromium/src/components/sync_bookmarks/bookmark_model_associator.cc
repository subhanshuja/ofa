// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync_bookmarks/bookmark_model_associator.h"

#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/format_macros.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/bookmarks/browser/bookmark_client.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/sync/api/sync_error.h"
#include "components/sync/api/sync_merge_result.h"
#include "components/sync/base/cryptographer.h"
#include "components/sync/base/data_type_histogram.h"
#include "components/sync/core/delete_journal.h"
#include "components/sync/core/read_node.h"
#include "components/sync/core/read_transaction.h"
#include "components/sync/core/write_node.h"
#include "components/sync/core/write_transaction.h"
#include "components/sync/core_impl/syncapi_internal.h"
#include "components/sync/driver/sync_client.h"
#include "components/sync/syncable/entry.h"
#include "components/sync/syncable/syncable_write_transaction.h"
#include "components/sync_bookmarks/bookmark_change_processor.h"
#include "components/undo/bookmark_undo_service.h"
#include "components/undo/bookmark_undo_utils.h"

#if defined(OPERA_SYNC)
#include <utility>

#include "base/callback_helpers.h"
#include "components/sync/util/opera_tag_node_mapping.h"

#include "common/bookmarks/duplicate_tracker.h"
#include "common/sync/sync_config.h"
#include "common/sync/sync_duplication_debugging.h"
#include "components/bookmarks/browser/opera_bookmark_utils.h"
#endif  // OPERA_SYNC

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

namespace sync_bookmarks {

// The sync protocol identifies top-level entities by means of well-known tags,
// which should not be confused with titles.  Each tag corresponds to a
// singleton instance of a particular top-level node in a user's share; the
// tags are consistent across users. The tags allow us to locate the specific
// folders whose contents we care about synchronizing, without having to do a
// lookup by name or path.  The tags should not be made user-visible.
// For example, the tag "bookmark_bar" represents the permanent node for
// bookmarks bar in Chrome. The tag "other_bookmarks" represents the permanent
// folder Other Bookmarks in Chrome.
//
// It is the responsibility of something upstream (at time of writing,
// the sync server) to create these tagged nodes when initializing sync
// for the first time for a user.  Thus, once the backend finishes
// initializing, the ProfileSyncService can rely on the presence of tagged
// nodes.
//
// TODO(ncarter): Pull these tags from an external protocol specification
// rather than hardcoding them here.
const char kBookmarkBarTag[] = "bookmark_bar";
const char kMobileBookmarksTag[] = "synced_bookmarks";
const char kOtherBookmarksTag[] = "other_bookmarks";

// Maximum number of bytes to allow in a title (must match sync's internal
// limits; see write_node.cc).
const int kTitleLimitBytes = 255;

#if defined(OPERA_SYNC)
// Returns a name of an item on sync server that corresponds to the name of a
// custom node.
std::string SyncItemTagFromNodeName(const std::string& node_name) {
  opera_sync::TagNodeMappingVector v = opera_sync::tag_node_mapping();
  for (auto it = v.begin(); it != v.end(); it++)
    if (node_name == it->node_name)
      return it->tag_name;
  return "";
}
#endif  // OPERA_SYNC

// Provides the following abstraction: given a parent bookmark node, find best
// matching child node for many sync nodes.
class BookmarkNodeFinder {
 public:
  // Creates an instance with the given parent bookmark node.
  explicit BookmarkNodeFinder(const BookmarkNode* parent_node);

  // Finds the bookmark node that matches the given url, title and folder
  // attribute. Returns the matching node if one exists; NULL otherwise.
  // If there are multiple matches then a node with ID matching |preferred_id|
  // is returned; otherwise the first matching node is returned.
  // If a matching node is found, it's removed for further matches.
  // If guid is empty it's ignored, if non-empty the returned node must have
  // a matching guid
  const BookmarkNode* FindBookmarkNode(const GURL& url,
                                       const std::string& title,
#if defined(OPERA_SYNC)
                                       const std::string& guid,
#endif  // OPERA_SYNC
                                       bool is_folder,
                                       int64_t preferred_id);

  // Returns true if |bookmark_node| matches the specified |url|,
  // |title|, and |is_folder| flags.
  static bool NodeMatches(const BookmarkNode* bookmark_node,
                          const GURL& url,
                          const std::string& title,
                          bool is_folder);

 private:
  // Maps bookmark node titles to instances, duplicates allowed.
  // Titles are converted to the sync internal format before
  // being used as keys for the map.
  typedef base::hash_multimap<std::string,
                              const BookmarkNode*> BookmarkNodeMap;
  typedef std::pair<BookmarkNodeMap::iterator,
                    BookmarkNodeMap::iterator> BookmarkNodeRange;

  // Converts and truncates bookmark titles in the form sync does internally
  // to avoid mismatches due to sync munging titles.
  static void ConvertTitleToSyncInternalFormat(const std::string& input,
                                               std::string* output);

  const BookmarkNode* parent_node_;
  BookmarkNodeMap child_nodes_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkNodeFinder);
};

class ScopedAssociationUpdater {
 public:
  explicit ScopedAssociationUpdater(BookmarkModel* model) {
    model_ = model;
    model->BeginExtensiveChanges();
  }

  ~ScopedAssociationUpdater() {
    model_->EndExtensiveChanges();
  }

 private:
  BookmarkModel* model_;

  DISALLOW_COPY_AND_ASSIGN(ScopedAssociationUpdater);
};

BookmarkNodeFinder::BookmarkNodeFinder(const BookmarkNode* parent_node)
    : parent_node_(parent_node) {
  for (int i = 0; i < parent_node_->child_count(); ++i) {
    const BookmarkNode* child_node = parent_node_->GetChild(i);

    std::string title = base::UTF16ToUTF8(child_node->GetTitle());
    ConvertTitleToSyncInternalFormat(title, &title);

    child_nodes_.insert(std::make_pair(title, child_node));
  }
}

const BookmarkNode* BookmarkNodeFinder::FindBookmarkNode(
    const GURL& url,
    const std::string& title,
#if defined(OPERA_SYNC)
    const std::string& guid,
#endif  // OPERA_SYNC
    bool is_folder,
    int64_t preferred_id) {
  const BookmarkNode* match = nullptr;

  // First lookup a range of bookmarks with the same title.
  std::string adjusted_title;
  ConvertTitleToSyncInternalFormat(title, &adjusted_title);

  BookmarkNodeRange range = child_nodes_.equal_range(adjusted_title);
  BookmarkNodeMap::iterator match_iter = range.second;
  for (BookmarkNodeMap::iterator iter = range.first;
       iter != range.second;
       ++iter) {
    // Then within the range match the node by the folder bit
    // and the url.
    const BookmarkNode* node = iter->second;

#if defined(OPERA_SYNC)
    // Only allow nodes with same guid to match.
    // This is used by the special speeddial root folders that are unique
    // for each install. However, a device can have multiple installs or
    // multiple devices can have the same name. So the normal matching by
    // name to merge doesn't work so we need to make sure that it must match,
    // so the right speeddial root folder is merged.
    std::string node_guid;
    node->GetMetaInfo(opera::kBookmarkSpeedDialRootFolderGUIDKey, &node_guid);
    if (guid != node_guid)
      continue;
#endif  // OPERA_SYNC

    if (is_folder == node->is_folder() && url == node->url()) {
      if (node->id() == preferred_id || preferred_id == 0) {
        // Preferred match - use this node.
        match = node;
        match_iter = iter;
        break;
      } else if (match == nullptr) {
        // First match - continue iterating.
        match = node;
        match_iter = iter;
      }
    }
  }

  if (match_iter != range.second) {
    // Remove the matched node so we don't match with it again.
    child_nodes_.erase(match_iter);
  }

  return match;
}

/* static */
bool BookmarkNodeFinder::NodeMatches(const BookmarkNode* bookmark_node,
                                     const GURL& url,
                                     const std::string& title,
                                     bool is_folder) {
  if (url != bookmark_node->url() || is_folder != bookmark_node->is_folder()) {
    return false;
  }

  // The title passed to this method comes from a sync directory entry.
  // The following two lines are needed to make the native bookmark title
  // comparable. The same conversion is used in BookmarkNodeFinder constructor.
  std::string bookmark_title = base::UTF16ToUTF8(bookmark_node->GetTitle());
  ConvertTitleToSyncInternalFormat(bookmark_title, &bookmark_title);
  return title == bookmark_title;
}

/* static */
void BookmarkNodeFinder::ConvertTitleToSyncInternalFormat(
    const std::string& input, std::string* output) {
  syncer::SyncAPINameToServerName(input, output);
  base::TruncateUTF8ToByteSize(*output, kTitleLimitBytes, output);
}

BookmarkModelAssociator::Context::Context(
    syncer::SyncMergeResult* local_merge_result,
    syncer::SyncMergeResult* syncer_merge_result)
    : local_merge_result_(local_merge_result),
      syncer_merge_result_(syncer_merge_result),
      duplicate_count_(0),
      native_model_sync_state_(UNSET) {}

BookmarkModelAssociator::Context::~Context() {
}

void BookmarkModelAssociator::Context::PushNode(int64_t sync_id) {
  dfs_stack_.push(sync_id);
}

bool BookmarkModelAssociator::Context::PopNode(int64_t* sync_id) {
  if (dfs_stack_.empty()) {
    *sync_id = 0;
    return false;
  }
  *sync_id = dfs_stack_.top();
  dfs_stack_.pop();
  return true;
}

void BookmarkModelAssociator::Context::SetPreAssociationVersions(
    int64_t native_version,
    int64_t sync_version) {
  local_merge_result_->set_pre_association_version(native_version);
  syncer_merge_result_->set_pre_association_version(sync_version);
}

void BookmarkModelAssociator::Context::SetNumItemsBeforeAssociation(
    int local_num,
    int sync_num) {
  local_merge_result_->set_num_items_before_association(local_num);
  syncer_merge_result_->set_num_items_before_association(sync_num);
}

void BookmarkModelAssociator::Context::SetNumItemsAfterAssociation(
    int local_num,
    int sync_num) {
  local_merge_result_->set_num_items_after_association(local_num);
  syncer_merge_result_->set_num_items_after_association(sync_num);
}

void BookmarkModelAssociator::Context::IncrementLocalItemsDeleted() {
  local_merge_result_->set_num_items_deleted(
      local_merge_result_->num_items_deleted() + 1);
}

void BookmarkModelAssociator::Context::IncrementLocalItemsAdded() {
  local_merge_result_->set_num_items_added(
      local_merge_result_->num_items_added() + 1);
}

void BookmarkModelAssociator::Context::IncrementLocalItemsModified() {
  local_merge_result_->set_num_items_modified(
      local_merge_result_->num_items_modified() + 1);
}

void BookmarkModelAssociator::Context::IncrementSyncItemsAdded() {
  syncer_merge_result_->set_num_items_added(
      syncer_merge_result_->num_items_added() + 1);
}

void BookmarkModelAssociator::Context::IncrementSyncItemsDeleted(int count) {
  syncer_merge_result_->set_num_items_deleted(
      syncer_merge_result_->num_items_deleted() + count);
}

void BookmarkModelAssociator::Context::UpdateDuplicateCount(
    const base::string16& title,
    const GURL& url) {
  // base::Hash is defined for 8-byte strings only so have to
  // cast the title data to char* and double the length in order to
  // compute its hash.
  size_t bookmark_hash = base::Hash(reinterpret_cast<const char*>(title.data()),
                                    title.length() * 2) ^
                         base::Hash(url.spec());

  if (!hashes_.insert(bookmark_hash).second) {
    // This hash code already exists in the set.
    ++duplicate_count_;
  }
}

void BookmarkModelAssociator::Context::AddBookmarkRoot(
    const bookmarks::BookmarkNode* root) {
  bookmark_roots_.push_back(root);
}

int64_t BookmarkModelAssociator::Context::GetSyncPreAssociationVersion() const {
  return syncer_merge_result_->pre_association_version();
}

void BookmarkModelAssociator::Context::MarkForVersionUpdate(
    const bookmarks::BookmarkNode* node) {
  bookmarks_for_version_update_.push_back(node);
}

BookmarkModelAssociator::BookmarkModelAssociator(
    BookmarkModel* bookmark_model,
    syncer::SyncClient* sync_client,
    syncer::UserShare* user_share,
    std::unique_ptr<syncer::DataTypeErrorHandler> unrecoverable_error_handler,
    bool expect_mobile_bookmarks_folder)
    : bookmark_model_(bookmark_model),
      sync_client_(sync_client),
      user_share_(user_share),
      unrecoverable_error_handler_(std::move(unrecoverable_error_handler)),
      expect_mobile_bookmarks_folder_(expect_mobile_bookmarks_folder),
      weak_factory_(this) {
  DCHECK(bookmark_model_);
  DCHECK(user_share_);
  DCHECK(unrecoverable_error_handler_);
}

BookmarkModelAssociator::~BookmarkModelAssociator() {
  DCHECK(thread_checker_.CalledOnValidThread());
#if defined(OPERA_SYNC)
  if (opera::SyncConfig::ShouldDeduplicateBookmarks())
    sync_client_->GetBookmarkDuplicateTracker()->SetBookmarkModelAssociator(
        nullptr);
#endif  // OPERA_SYNC
}

syncer::SyncError BookmarkModelAssociator::DisassociateModels() {
  id_map_.clear();
  id_map_inverse_.clear();
  dirty_associations_sync_ids_.clear();
  return syncer::SyncError();
}

int64_t BookmarkModelAssociator::GetSyncIdFromChromeId(const int64_t& node_id) {
  BookmarkIdToSyncIdMap::const_iterator iter = id_map_.find(node_id);
  return iter == id_map_.end() ? syncer::kInvalidId : iter->second;
}

const BookmarkNode* BookmarkModelAssociator::GetChromeNodeFromSyncId(
    int64_t sync_id) {
  SyncIdToBookmarkNodeMap::const_iterator iter = id_map_inverse_.find(sync_id);
  return iter == id_map_inverse_.end() ? NULL : iter->second;
}

bool BookmarkModelAssociator::InitSyncNodeFromChromeId(
    const int64_t& node_id,
    syncer::BaseNode* sync_node) {
  DCHECK(sync_node);
  int64_t sync_id = GetSyncIdFromChromeId(node_id);
  if (sync_id == syncer::kInvalidId)
    return false;
  if (sync_node->InitByIdLookup(sync_id) != syncer::BaseNode::INIT_OK)
    return false;
  DCHECK(sync_node->GetId() == sync_id);
  return true;
}

void BookmarkModelAssociator::AddAssociation(const BookmarkNode* node,
                                             int64_t sync_id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  int64_t node_id = node->id();
  DCHECK_NE(sync_id, syncer::kInvalidId);
  DCHECK(id_map_.find(node_id) == id_map_.end());
  DCHECK(id_map_inverse_.find(sync_id) == id_map_inverse_.end());
  id_map_[node_id] = sync_id;
  id_map_inverse_[sync_id] = node;
}

void BookmarkModelAssociator::Associate(const BookmarkNode* node,
                                        const syncer::BaseNode& sync_node) {
  AddAssociation(node, sync_node.GetId());

  // The same check exists in PersistAssociations. However it is better to
  // do the check earlier to avoid the cost of decrypting nodes again
  // in PersistAssociations.
  if (node->id() != sync_node.GetExternalId()) {
    dirty_associations_sync_ids_.insert(sync_node.GetId());
    PostPersistAssociationsTask();
  }
}

void BookmarkModelAssociator::Disassociate(int64_t sync_id) {
  DCHECK(thread_checker_.CalledOnValidThread());
  SyncIdToBookmarkNodeMap::iterator iter = id_map_inverse_.find(sync_id);
  if (iter == id_map_inverse_.end())
    return;
  id_map_.erase(iter->second->id());
  id_map_inverse_.erase(iter);
  dirty_associations_sync_ids_.erase(sync_id);
}

bool BookmarkModelAssociator::SyncModelHasUserCreatedNodes(bool* has_nodes) {
  DCHECK(has_nodes);
  *has_nodes = false;
  bool has_mobile_folder = true;

  syncer::ReadTransaction trans(FROM_HERE, user_share_);

#if defined(OPERA_SYNC)
  bool custom_folders_have_children = false;
  opera_sync::TagNodeMappingVector opera_mapping =
      opera_sync::tag_node_mapping();
  for (auto it = opera_mapping.begin(); it != opera_mapping.end(); it++) {
    if (custom_folders_have_children)
      continue;
    syncer::ReadNode custom_node(&trans);
    if (custom_node.InitByTagLookupForBookmarks(it->tag_name) !=
        syncer::BaseNode::INIT_OK) {
      return false;
    }
    if (custom_node.HasChildren())
      custom_folders_have_children = true;
  }
#endif  // OPERA_SYNC

  syncer::ReadNode bookmark_bar_node(&trans);
  if (bookmark_bar_node.InitByTagLookupForBookmarks(kBookmarkBarTag) !=
      syncer::BaseNode::INIT_OK) {
    return false;
  }

  syncer::ReadNode other_bookmarks_node(&trans);
  if (other_bookmarks_node.InitByTagLookupForBookmarks(kOtherBookmarksTag) !=
      syncer::BaseNode::INIT_OK) {
    return false;
  }

  syncer::ReadNode mobile_bookmarks_node(&trans);
  if (mobile_bookmarks_node.InitByTagLookupForBookmarks(kMobileBookmarksTag) !=
      syncer::BaseNode::INIT_OK) {
    has_mobile_folder = false;
  }

  // Sync model has user created nodes if any of the permanent nodes has
  // children.
  *has_nodes = bookmark_bar_node.HasChildren() ||
      other_bookmarks_node.HasChildren() ||
#if defined(OPERA_SYNC)
      custom_folders_have_children ||
#endif  // OPERA_SYNC
      (has_mobile_folder && mobile_bookmarks_node.HasChildren());
  return true;
}

bool BookmarkModelAssociator::AssociateTaggedPermanentNode(
    syncer::BaseTransaction* trans,
    const BookmarkNode* permanent_node,
    const std::string& tag) {
  // Do nothing if |permanent_node| is already initialized and associated.
  int64_t sync_id = GetSyncIdFromChromeId(permanent_node->id());
  if (sync_id != syncer::kInvalidId)
    return true;

  syncer::ReadNode sync_node(trans);
  if (sync_node.InitByTagLookupForBookmarks(tag) != syncer::BaseNode::INIT_OK)
    return false;

  Associate(permanent_node, sync_node);
  return true;
}

syncer::SyncError BookmarkModelAssociator::AssociateModels(
    syncer::SyncMergeResult* local_merge_result,
    syncer::SyncMergeResult* syncer_merge_result) {
  // Since any changes to the bookmark model made here are not user initiated,
  // these change should not be undoable and so suspend the undo tracking.
  ScopedSuspendBookmarkUndo suspend_undo(
      sync_client_->GetBookmarkUndoServiceIfExists());

  Context context(local_merge_result, syncer_merge_result);

  syncer::SyncError error = CheckModelSyncState(&context);
  if (error.IsSet())
    return error;

  std::unique_ptr<ScopedAssociationUpdater> association_updater(
      new ScopedAssociationUpdater(bookmark_model_));
  DisassociateModels();

  error = BuildAssociations(&context);
  if (error.IsSet()) {
    // Clear version on bookmark model so that the conservative association
    // algorithm is used on the next association.
    bookmark_model_->SetNodeSyncTransactionVersion(
        bookmark_model_->root_node(),
        syncer::syncable::kInvalidTransactionVersion);
#if defined(OPERA_SYNC)
  } else {
    if (opera::SyncConfig::ShouldDeduplicateBookmarks())
      sync_client_->GetBookmarkDuplicateTracker()->SetBookmarkModelAssociator(
          this);
#endif  // OPERA_SYNC
  }

  return error;
}

syncer::SyncError BookmarkModelAssociator::AssociatePermanentFolders(
    syncer::BaseTransaction* trans,
    Context* context) {
  // To prime our association, we associate the top-level nodes, Bookmark Bar
  // and Other Bookmarks.
  if (!AssociateTaggedPermanentNode(trans, bookmark_model_->bookmark_bar_node(),
                                    kBookmarkBarTag)) {
    return unrecoverable_error_handler_->CreateAndUploadError(
        FROM_HERE, "Bookmark bar node not found", model_type());
  }

  if (!AssociateTaggedPermanentNode(trans, bookmark_model_->other_node(),
                                    kOtherBookmarksTag)) {
    return unrecoverable_error_handler_->CreateAndUploadError(
        FROM_HERE, "Other bookmarks node not found", model_type());
  }

  if (!AssociateTaggedPermanentNode(trans, bookmark_model_->mobile_node(),
                                    kMobileBookmarksTag) &&
      expect_mobile_bookmarks_folder_) {
    return unrecoverable_error_handler_->CreateAndUploadError(
        FROM_HERE, "Mobile bookmarks node not found", model_type());
  }

#if !defined(OPERA_SYNC)
  // Note: the root node may have additional extra nodes. Currently none of
  // them are meant to sync.
#else
  // Associate custom nodes (if any exist)
  const bookmarks::CustomBookmarkNodes& custom_permanent_nodes =
      bookmark_model_->custom_nodes();
  for (bookmarks::CustomBookmarkNodes::const_iterator i =
           custom_permanent_nodes.begin();
       i != custom_permanent_nodes.end();
       i++) {
    if (!AssociateTaggedPermanentNode(trans, i->second,
                                      SyncItemTagFromNodeName(i->first))) {
      return unrecoverable_error_handler_->CreateAndUploadError(
          FROM_HERE,
          i->first + " bookmarks node not found",
          model_type());
    }
  }
#endif  // !OPERA_SYNC
  int64_t bookmark_bar_sync_id =
      GetSyncIdFromChromeId(bookmark_model_->bookmark_bar_node()->id());
  DCHECK_NE(bookmark_bar_sync_id, syncer::kInvalidId);
  context->AddBookmarkRoot(bookmark_model_->bookmark_bar_node());
  int64_t other_bookmarks_sync_id =
      GetSyncIdFromChromeId(bookmark_model_->other_node()->id());
  DCHECK_NE(other_bookmarks_sync_id, syncer::kInvalidId);
  context->AddBookmarkRoot(bookmark_model_->other_node());
  int64_t mobile_bookmarks_sync_id =
      GetSyncIdFromChromeId(bookmark_model_->mobile_node()->id());
  if (expect_mobile_bookmarks_folder_)
    DCHECK_NE(syncer::kInvalidId, mobile_bookmarks_sync_id);
  if (mobile_bookmarks_sync_id != syncer::kInvalidId)
    context->AddBookmarkRoot(bookmark_model_->mobile_node());

  // WARNING: The order in which we push these should match their order in the
  // bookmark model (see BookmarkModel::DoneLoading(..)).
  context->PushNode(bookmark_bar_sync_id);
  context->PushNode(other_bookmarks_sync_id);
  if (mobile_bookmarks_sync_id != syncer::kInvalidId)
    context->PushNode(mobile_bookmarks_sync_id);

#if defined(OPERA_SYNC)
  // Custom nodes
  for (bookmarks::CustomBookmarkNodes::const_iterator i =
           custom_permanent_nodes.begin();
       i != custom_permanent_nodes.end(); i++) {
    int64_t custom_root_node_id = GetSyncIdFromChromeId(i->second->id());
    DCHECK_NE(syncer::kInvalidId, custom_root_node_id);
    context->AddBookmarkRoot(i->second);
    context->PushNode(custom_root_node_id);
  }
#endif  // OPERA_SYNC
  return syncer::SyncError();
}

void BookmarkModelAssociator::SetNumItemsBeforeAssociation(
    syncer::BaseTransaction* trans,
    Context* context) {
  int syncer_num = 0;
  syncer::ReadNode bm_root(trans);
  if (bm_root.InitTypeRoot(syncer::BOOKMARKS) == syncer::BaseNode::INIT_OK) {
    syncer_num = bm_root.GetTotalNodeCount();
  }
  context->SetNumItemsBeforeAssociation(
      GetTotalBookmarkCountAndRecordDuplicates(bookmark_model_->root_node(),
                                               context),
      syncer_num);
}

int BookmarkModelAssociator::GetTotalBookmarkCountAndRecordDuplicates(
    const bookmarks::BookmarkNode* node,
    Context* context) const {
  int count = 1;  // Start with one to include the node itself.

  if (!node->is_root()) {
    context->UpdateDuplicateCount(node->GetTitle(), node->url());
  }

  for (int i = 0; i < node->child_count(); ++i) {
    count +=
        GetTotalBookmarkCountAndRecordDuplicates(node->GetChild(i), context);
  }

  return count;
}

void BookmarkModelAssociator::SetNumItemsAfterAssociation(
    syncer::BaseTransaction* trans,
    Context* context) {
  int syncer_num = 0;
  syncer::ReadNode bm_root(trans);
  if (bm_root.InitTypeRoot(syncer::BOOKMARKS) == syncer::BaseNode::INIT_OK) {
    syncer_num = bm_root.GetTotalNodeCount();
  }
  context->SetNumItemsAfterAssociation(
      bookmark_model_->root_node()->GetTotalNodeCount(), syncer_num);
}

syncer::SyncError BookmarkModelAssociator::BuildAssociations(Context* context) {
  DCHECK(bookmark_model_->loaded());
  DCHECK_NE(context->native_model_sync_state(), AHEAD);
#if defined(OPERA_SYNC)
  SYNC_DUPLICATION_LOG("BookmarkModelAssociator::BuildAssociations starts");
#endif

  int initial_duplicate_count = 0;
  int64_t new_version = syncer::syncable::kInvalidTransactionVersion;
  {
    syncer::WriteTransaction trans(FROM_HERE, user_share_, &new_version);

    syncer::SyncError error = AssociatePermanentFolders(&trans, context);
    if (error.IsSet())
      return error;

    SetNumItemsBeforeAssociation(&trans, context);
    initial_duplicate_count = context->duplicate_count();

#if !defined(OPERA_SYNC)
    // Remove obsolete bookmarks according to sync delete journal.
    // TODO(stanisc): crbug.com/456876: rewrite this to avoid a separate
    // traversal and instead perform deletes at the end of the loop below where
    // the unmatched bookmark nodes are created as sync nodes.
    ApplyDeletesFromSyncJournal(&trans, context);
#else  // OPERA_SYNC
    // Mark candidates for obsolete bookmarks according to sync delete journal.
    std::unique_ptr<BookmarkList> for_removal(
        ApplyDeletesFromSyncJournal(&trans, context));
    // Remember to always delete the entries in for_removal once the association
    // ends, ie. when this function exits.
    base::ScopedClosureRunner run_at_exit(
        base::Bind(base::IgnoreResult(&BookmarkModelAssociator::DropMarked),
                   base::Unretained(this), bookmark_model_->root_node(),
                   for_removal.get(), context));
#endif

    // Algorithm description:
    // Match up the roots and recursively do the following:
    // * For each sync node for the current sync parent node, find the best
    //   matching bookmark node under the corresponding bookmark parent node.
    //   If no matching node is found, create a new bookmark node in the same
    //   position as the corresponding sync node.
    //   If a matching node is found, update the properties of it from the
    //   corresponding sync node.
    // * When all children sync nodes are done, add the extra children bookmark
    //   nodes to the sync parent node.
    //
    // The best match algorithm uses folder title or bookmark title/url to
    // perform the primary match. If there are multiple match candidates it
    // selects the preferred one based on sync node external ID match to the
    // bookmark folder ID.
    int64_t sync_parent_id;
    while (context->PopNode(&sync_parent_id)) {
      syncer::ReadNode sync_parent(&trans);
      if (sync_parent.InitByIdLookup(sync_parent_id) !=
          syncer::BaseNode::INIT_OK) {
        return unrecoverable_error_handler_->CreateAndUploadError(
            FROM_HERE, "Failed to lookup node.", model_type());
      }
      // Only folder nodes are pushed on to the stack.
      DCHECK(sync_parent.GetIsFolder());

      const BookmarkNode* parent_node = GetChromeNodeFromSyncId(sync_parent_id);
      if (!parent_node) {
        return unrecoverable_error_handler_->CreateAndUploadError(
            FROM_HERE, "Failed to find bookmark node for sync id.",
            model_type());
      }
      DCHECK(parent_node->is_folder());

#if defined(OPERA_SYNC)
    std::map<int64_t, int> original_positions_for_ids;
    for (int i = 0; i < parent_node->child_count(); i++) {
      original_positions_for_ids[parent_node->GetChild(i)->id()] = i;
    }
#endif  // OPERA_SYNC

      std::vector<int64_t> children;
      sync_parent.GetChildIds(&children);

#if defined(OPERA_SYNC)
      error = BuildAssociations(&trans, parent_node, children, context,
                                for_removal.get(),
                                &original_positions_for_ids,
                                &sync_parent);
#else
      error = BuildAssociations(&trans, parent_node, children, context);
#endif  // OPERA_SYNC
      if (error.IsSet())
        return error;
    }

    SetNumItemsAfterAssociation(&trans, context);
  }

  if (new_version == syncer::syncable::kInvalidTransactionVersion) {
    // If we get here it means that none of Sync nodes were modified by the
    // association process.
    // We need to set |new_version| to the pre-association Sync version;
    // otherwise BookmarkChangeProcessor::UpdateTransactionVersion call below
    // won't save it to the native model. That is necessary to ensure that the
    // native model doesn't get stuck at "unset" version and skips any further
    // version checks.
    new_version = context->GetSyncPreAssociationVersion();
  }

  BookmarkChangeProcessor::UpdateTransactionVersion(
      new_version, bookmark_model_, context->bookmarks_for_version_update());

  UMA_HISTOGRAM_COUNTS("Sync.BookmarksDuplicationsAtAssociation",
                       context->duplicate_count());
  UMA_HISTOGRAM_COUNTS("Sync.BookmarksNewDuplicationsAtAssociation",
                       context->duplicate_count() - initial_duplicate_count);

  if (context->duplicate_count() > initial_duplicate_count) {
    UMA_HISTOGRAM_ENUMERATION("Sync.BookmarksModelSyncStateAtNewDuplication",
                              context->native_model_sync_state(),
                              NATIVE_MODEL_SYNC_STATE_COUNT);
  }
#if defined(OPERA_SYNC)
  SYNC_DUPLICATION_LOG("BookmarkModelAssociator::BuildAssociations ends");
#endif

  return syncer::SyncError();
}

syncer::SyncError BookmarkModelAssociator::BuildAssociations(
    syncer::WriteTransaction* trans,
    const BookmarkNode* parent_node,
    const std::vector<int64_t>& sync_ids,
#if defined(OPERA_SYNC)
    Context* context,
    BookmarkList* for_removal,
    std::map<int64_t, int>* original_positions_for_ids,
    syncer::ReadNode* sync_parent) {
  DCHECK(for_removal);
  DCHECK(original_positions_for_ids);
#else
    Context* context) {
#endif  // OPERA_SYNC
  BookmarkNodeFinder node_finder(parent_node);

  int index = 0;
  for (std::vector<int64_t>::const_iterator it = sync_ids.begin();
       it != sync_ids.end(); ++it) {
    int64_t sync_child_id = *it;
    syncer::ReadNode sync_child_node(trans);
    if (sync_child_node.InitByIdLookup(sync_child_id) !=
        syncer::BaseNode::INIT_OK) {
      return unrecoverable_error_handler_->CreateAndUploadError(
          FROM_HERE, "Failed to lookup node.", model_type());
    }

#if defined(OPERA_SYNC)
    const sync_pb::BookmarkSpecifics& specifics =
        sync_child_node.GetBookmarkSpecifics();
    std::string guid;
    for (int i = 0; i < specifics.meta_info_size(); i++) {
      if (specifics.meta_info(i).key() ==
          opera::kBookmarkSpeedDialRootFolderGUIDKey) {
        guid = specifics.meta_info(i).value();
        break;
      }
    }
#endif  // OPERA_SYNC

    int64_t external_id = sync_child_node.GetExternalId();
    GURL url(sync_child_node.GetBookmarkSpecifics().url());
    const BookmarkNode* child_node = node_finder.FindBookmarkNode(
#if defined(OPERA_SYNC)
        url, sync_child_node.GetTitle(), guid, sync_child_node.GetIsFolder(),
#else
        url, sync_child_node.GetTitle(), sync_child_node.GetIsFolder(),
#endif
        external_id);
    if (child_node) {
      // Skip local node update if the local model version matches and
      // the node is already associated and in the right position.
      bool is_in_sync = (context->native_model_sync_state() == IN_SYNC) &&
                        (child_node->id() == external_id) &&
                        (index < parent_node->child_count()) &&
                        (parent_node->GetChild(index) == child_node);
      if (!is_in_sync) {
        BookmarkChangeProcessor::UpdateBookmarkWithSyncData(
            sync_child_node, bookmark_model_, child_node, sync_client_);
        bookmark_model_->Move(child_node, parent_node, index);
        context->IncrementLocalItemsModified();
        context->MarkForVersionUpdate(child_node);
      }
    } else {
      syncer::SyncError error;
      child_node = CreateBookmarkNode(parent_node, index, &sync_child_node, url,
                                      context, &error);
      if (!child_node) {
        if (error.IsSet()) {
#if defined(OPERA_SYNC)
          SYNC_DUPLICATION_LOG("Couldn't create a node: " + error.ToString());
#endif  // OPERA_SYNC
          return error;
        } else {
#if defined(OPERA_SYNC)
          SYNC_DUPLICATION_LOG("Couldn't create a node, error not set");
#endif  // OPERA_SYNC
          // Skip this node and continue. Don't increment index in this case.
          continue;
        }
      }
      context->IncrementLocalItemsAdded();
      context->MarkForVersionUpdate(child_node);
    }

    Associate(child_node, sync_child_node);

    if (sync_child_node.GetIsFolder())
      context->PushNode(sync_child_id);
    ++index;
  }

  // At this point all the children nodes of the parent sync node have
  // corresponding children in the parent bookmark node and they are all in
  // the right positions: from 0 to index - 1.
  // So the children starting from index in the parent bookmark node are the
  // ones that are not present in the parent sync node. So create them.
#if defined(OPERA_SYNC)
  // Later on we will move them around to restore their original positions,
  // so we will keep original-to-current position map. We use that mapping
  // direction to make sure that order of restorations will be the same as the
  // order of original positions.
  std::map<int, int> child_positions_to_restore;
#endif  // OPERA_SYNC
  for (int i = index; i < parent_node->child_count(); ++i) {
#if defined(OPERA_SYNC)
    child_positions_to_restore
        [(*original_positions_for_ids)[parent_node->GetChild(i)->id()]] = i;
#endif  // OPERA_SYNC
    int64_t sync_child_id = BookmarkChangeProcessor::CreateSyncNode(
        parent_node, bookmark_model_, i, trans, this,
        unrecoverable_error_handler_.get());
    if (syncer::kInvalidId == sync_child_id) {
      return unrecoverable_error_handler_->CreateAndUploadError(
          FROM_HERE, "Failed to create sync node.", model_type());
    }

    context->IncrementSyncItemsAdded();
    const BookmarkNode* child_node = parent_node->GetChild(i);
    context->MarkForVersionUpdate(child_node);
    if (child_node->is_folder())
      context->PushNode(sync_child_id);
  }

#if defined(OPERA_SYNC)
  // Restoring new bookmarks to their pre-sync positions. Since we cannot rely
  // on BookmarkChangeProcessor (dependency is the other way round) we have to
  // manually change sync node position to match bookmark's one.
  for (const auto& original_current : child_positions_to_restore) {
    const int original_index = original_current.first;
    const int current_index = original_current.second;
    const auto* child_node = parent_node->GetChild(current_index);
    bookmark_model_->Move(child_node, parent_node, original_index);

    syncer::WriteNode sync_child_node(trans);
    if (!InitSyncNodeFromChromeId(child_node->id(), &sync_child_node))
      continue;

    if (original_index == 0) {
      sync_child_node.SetPosition(*sync_parent, nullptr);
    } else {
      const auto* prev_node = parent_node->GetChild(original_index - 1);
      syncer::ReadNode sync_prev_node(trans);
      if (InitSyncNodeFromChromeId(prev_node->id(), &sync_prev_node))
        sync_child_node.SetPosition(*sync_parent, &sync_prev_node);
    }
  }
#endif  // OPERA_SYNC

  return syncer::SyncError();
}

const BookmarkNode* BookmarkModelAssociator::CreateBookmarkNode(
    const BookmarkNode* parent_node,
    int bookmark_index,
    const syncer::BaseNode* sync_child_node,
    const GURL& url,
    Context* context,
    syncer::SyncError* error) {
  DCHECK_LE(bookmark_index, parent_node->child_count());

  const std::string& sync_title = sync_child_node->GetTitle();

  if (!sync_child_node->GetIsFolder() && !url.is_valid()) {
#if defined(OPERA_SYNC)
    SYNC_DUPLICATION_LOG("Node's URL is invalid");
#endif  // OPERA_SYNC
    unrecoverable_error_handler_->CreateAndUploadError(
        FROM_HERE,
        "Cannot associate sync node " + sync_child_node->GetSyncId().value() +
            " with invalid url " + url.possibly_invalid_spec() + " and title " +
            sync_title,
        model_type());
    // Don't propagate the error to the model_type in this case.
    return nullptr;
  }

  base::string16 bookmark_title = base::UTF8ToUTF16(sync_title);
  const BookmarkNode* child_node = BookmarkChangeProcessor::CreateBookmarkNode(
      bookmark_title, url, sync_child_node, parent_node, bookmark_model_,
      sync_client_, bookmark_index);
  if (!child_node) {
#if defined(OPERA_SYNC)
    SYNC_DUPLICATION_LOG("Could not create node");
#endif  // OPERA_SYNC
    *error = unrecoverable_error_handler_->CreateAndUploadError(
        FROM_HERE, "Failed to create bookmark node with title " + sync_title +
                       " and url " + url.possibly_invalid_spec(),
        model_type());
    return nullptr;
  }

  context->UpdateDuplicateCount(bookmark_title, url);
  return child_node;
}

int BookmarkModelAssociator::RemoveSyncNodeHierarchy(
    syncer::WriteTransaction* trans,
    int64_t sync_id) {
  syncer::WriteNode sync_node(trans);
  if (sync_node.InitByIdLookup(sync_id) != syncer::BaseNode::INIT_OK) {
    syncer::SyncError error(FROM_HERE, syncer::SyncError::DATATYPE_ERROR,
                            "Could not lookup bookmark node for ID deletion.",
                            syncer::BOOKMARKS);
    unrecoverable_error_handler_->OnUnrecoverableError(error);
    return 0;
  }

  return BookmarkChangeProcessor::RemoveSyncNodeHierarchy(trans, &sync_node,
                                                          this);
}

struct FolderInfo {
  FolderInfo(const BookmarkNode* f, const BookmarkNode* p, int64_t id)
      : folder(f), parent(p), sync_id(id) {}
  const BookmarkNode* folder;
  const BookmarkNode* parent;
  int64_t sync_id;
};
typedef std::vector<FolderInfo> FolderInfoList;

#if defined(OPERA_SYNC)
std::unique_ptr<BookmarkModelAssociator::BookmarkList>
BookmarkModelAssociator::ApplyDeletesFromSyncJournal(
    syncer::BaseTransaction* trans,
    Context* context) {
#else
void BookmarkModelAssociator::ApplyDeletesFromSyncJournal(
    syncer::BaseTransaction* trans,
    Context* context) {
#endif  // OPERA_SYNC
#if defined(OPERA_SYNC)
  std::unique_ptr<BookmarkList> for_removal(new BookmarkList);
#endif  // OPERA_SYNC

  syncer::BookmarkDeleteJournalList bk_delete_journals;
  syncer::DeleteJournal::GetBookmarkDeleteJournals(trans, &bk_delete_journals);
  if (bk_delete_journals.empty())
#if defined(OPERA_SYNC)
    return for_removal;
#else
    return;
#endif  // OPERA_SYNC
  size_t num_journals_unmatched = bk_delete_journals.size();

  // Make a set of all external IDs in the delete journal,
  // ignore entries with unset external IDs.
  std::set<int64_t> journaled_external_ids;
  for (size_t i = 0; i < num_journals_unmatched; i++) {
    if (bk_delete_journals[i].external_id != 0)
      journaled_external_ids.insert(bk_delete_journals[i].external_id);
  }

  // Check bookmark model from top to bottom.
  BookmarkStack dfs_stack;
  for (BookmarkList::const_iterator it = context->bookmark_roots().begin();
       it != context->bookmark_roots().end(); ++it) {
    dfs_stack.push(*it);
  }

  // Remember folders that match delete journals in first pass but don't delete
  // them in case there are bookmarks left under them. After non-folder
  // bookmarks are removed in first pass, recheck the folders in reverse order
  // to remove empty ones.
  FolderInfoList folders_matched;
  while (!dfs_stack.empty() && num_journals_unmatched > 0) {
    const BookmarkNode* parent = dfs_stack.top();
    dfs_stack.pop();
    DCHECK(parent->is_folder());

    // Enumerate folder children in reverse order to make it easier to remove
    // bookmarks matching entries in the delete journal.
    for (int child_index = parent->child_count() - 1;
         child_index >= 0 && num_journals_unmatched > 0; --child_index) {
      const BookmarkNode* child = parent->GetChild(child_index);
      if (child->is_folder())
        dfs_stack.push(child);

      if (journaled_external_ids.find(child->id()) ==
          journaled_external_ids.end()) {
        // Skip bookmark node which id is not in the set of external IDs.
        continue;
      }

      // Iterate through the journal entries from back to front. Remove matched
      // journal by moving an unmatched entry at the tail to the matched
      // position so that we can read unmatched entries off the head in next
      // loop.
      for (int journal_index = num_journals_unmatched - 1; journal_index >= 0;
           --journal_index) {
        const syncer::BookmarkDeleteJournal& delete_entry =
            bk_delete_journals[journal_index];
        if (child->id() == delete_entry.external_id &&
            BookmarkNodeFinder::NodeMatches(
                child, GURL(delete_entry.specifics.bookmark().url()),
                delete_entry.specifics.bookmark().title(),
                delete_entry.is_folder)) {
          if (child->is_folder()) {
            // Remember matched folder without removing and delete only empty
            // ones later.
            folders_matched.push_back(
                FolderInfo(child, parent, delete_entry.id));
          } else {
#if defined(OPERA_SYNC)
            // Do not actually remove the node here, just mark it for removal.
            // It is possible that a matching node will be found during
            // association with the sync model, and since we don't want to
            // remove the node just to recreate it right after, we merely
            // mark this node for removal here and go on with processing.
            for_removal->push_back(child);
#else
            bookmark_model_->Remove(child);
            context->IncrementLocalItemsDeleted();
#endif  // OPERA_SYNC
          }
          // Move unmatched journal here and decrement counter.
          bk_delete_journals[journal_index] =
              bk_delete_journals[--num_journals_unmatched];
          break;
        }
      }
    }
  }

  // Ids of sync nodes not found in bookmark model, meaning the deletions are
  // persisted and correponding delete journals can be dropped.
  std::set<int64_t> journals_to_purge;

  // Remove empty folders from bottom to top.
  for (FolderInfoList::reverse_iterator it = folders_matched.rbegin();
      it != folders_matched.rend(); ++it) {
    if (it->folder->child_count() == 0) {
#if defined(OPERA_SYNC)
      // Do not actually remove the node here, just mark it for removal.
      // It is possible that a matching node will be found during
      // association with the sync model, and since we don't want to
      // remove the node just to recreate it right after, we merely
      // mark this node for removal here and go on with processing.
      for_removal->push_back(it->folder);
#else
      bookmark_model_->Remove(it->folder);
      context->IncrementLocalItemsDeleted();
#endif  // OPERA_SYNC
    } else {
      // Keep non-empty folder and remove its journal so that it won't match
      // again in the future.
      journals_to_purge.insert(it->sync_id);
    }
  }

  // Purge unmatched journals.
  for (size_t i = 0; i < num_journals_unmatched; ++i)
    journals_to_purge.insert(bk_delete_journals[i].id);
  syncer::DeleteJournal::PurgeDeleteJournals(trans, journals_to_purge);

#if defined(OPERA_SYNC)
  return for_removal;
#endif  // OPERA_SYNC
}

void BookmarkModelAssociator::PostPersistAssociationsTask() {
  // No need to post a task if a task is already pending.
  if (weak_factory_.HasWeakPtrs())
    return;
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(&BookmarkModelAssociator::PersistAssociations,
                            weak_factory_.GetWeakPtr()));
}

void BookmarkModelAssociator::PersistAssociations() {
  // If there are no dirty associations we have nothing to do. We handle this
  // explicity instead of letting the for loop do it to avoid creating a write
  // transaction in this case.
  if (dirty_associations_sync_ids_.empty()) {
    DCHECK(id_map_.empty());
    DCHECK(id_map_inverse_.empty());
    return;
  }

  int64_t new_version = syncer::syncable::kInvalidTransactionVersion;
  std::vector<const BookmarkNode*> bnodes;
  {
    syncer::WriteTransaction trans(FROM_HERE, user_share_, &new_version);
    DirtyAssociationsSyncIds::iterator iter;
    for (iter = dirty_associations_sync_ids_.begin();
         iter != dirty_associations_sync_ids_.end();
         ++iter) {
      int64_t sync_id = *iter;
      syncer::WriteNode sync_node(&trans);
      if (sync_node.InitByIdLookup(sync_id) != syncer::BaseNode::INIT_OK) {
        syncer::SyncError error(
            FROM_HERE,
            syncer::SyncError::DATATYPE_ERROR,
            "Could not lookup bookmark node for ID persistence.",
            syncer::BOOKMARKS);
        unrecoverable_error_handler_->OnUnrecoverableError(error);
        return;
      }
      const BookmarkNode* node = GetChromeNodeFromSyncId(sync_id);
      if (node && sync_node.GetExternalId() != node->id()) {
        sync_node.SetExternalId(node->id());
        bnodes.push_back(node);
      }
    }
    dirty_associations_sync_ids_.clear();
  }

  BookmarkChangeProcessor::UpdateTransactionVersion(new_version,
                                                    bookmark_model_,
                                                    bnodes);
}

bool BookmarkModelAssociator::CryptoReadyIfNecessary() {
  // We only access the cryptographer while holding a transaction.
  syncer::ReadTransaction trans(FROM_HERE, user_share_);
  const syncer::ModelTypeSet encrypted_types = trans.GetEncryptedTypes();
  return !encrypted_types.Has(syncer::BOOKMARKS) ||
      trans.GetCryptographer()->is_ready();
}

syncer::SyncError BookmarkModelAssociator::CheckModelSyncState(
    Context* context) const {
  DCHECK_EQ(context->native_model_sync_state(), UNSET);
  int64_t native_version =
      bookmark_model_->root_node()->sync_transaction_version();

  syncer::ReadTransaction trans(FROM_HERE, user_share_);
  int64_t sync_version = trans.GetModelVersion(syncer::BOOKMARKS);
  context->SetPreAssociationVersions(native_version, sync_version);

  if (native_version != syncer::syncable::kInvalidTransactionVersion) {
    if (native_version == sync_version) {
      context->set_native_model_sync_state(IN_SYNC);
    } else {
      UMA_HISTOGRAM_ENUMERATION("Sync.LocalModelOutOfSync",
                                ModelTypeToHistogramInt(syncer::BOOKMARKS),
                                syncer::MODEL_TYPE_COUNT);

      // Clear version on bookmark model so that we only report error once.
      bookmark_model_->SetNodeSyncTransactionVersion(
          bookmark_model_->root_node(),
          syncer::syncable::kInvalidTransactionVersion);

      // If the native version is higher, there was a sync persistence failure,
      // and we need to delay association until after a GetUpdates.
      if (native_version > sync_version) {
        context->set_native_model_sync_state(AHEAD);
        std::string message = base::StringPrintf(
            "Native version (%" PRId64 ") does not match sync version (%"
                PRId64 ")",
            native_version,
            sync_version);
        return syncer::SyncError(FROM_HERE,
                                 syncer::SyncError::PERSISTENCE_ERROR,
                                 message,
                                 syncer::BOOKMARKS);
      } else {
        context->set_native_model_sync_state(BEHIND);
      }
    }
  }
  return syncer::SyncError();
}

#if defined(OPERA_SYNC)
int64_t BookmarkModelAssociator::DropMarked(const BookmarkNode* start,
                                            BookmarkList* for_removal,
                                            Context* context) {
  DCHECK(start);
  DCHECK(for_removal);
  DCHECK(context);

  int64_t deleted_count = 0;
  // Actually do remove all nodes that are still marked for removal.
  // The nodes that are still marked are the nodes that were marked
  // by the journal and not un-marked during association, meaning that
  // these are nodes that:
  // a) were found in the local bookmark model on browser startup and
  // b) were marked as deleted during previous browser session thus
  //    are found in the journal and
  // c) were not associated with any sync node during initial models
  //    association.
  for (int i = 0; i < start->child_count(); i++) {
    const BookmarkNode* child = start->GetChild(i);
    DCHECK(child);

    if (child->is_folder())
      deleted_count += DropMarked(child, for_removal, context);

    BookmarkList::iterator it =
        std::find(for_removal->begin(), for_removal->end(), child);
    if (it != for_removal->end()) {
      const BookmarkNode* parent = child->parent();
      DCHECK(parent);
      deleted_count++;
      context->IncrementLocalItemsDeleted();
      bookmark_model_->Remove(child);
    }
  }

  return deleted_count;
}
#endif  // OPERA_SYNC

}  // namespace sync_bookmarks
