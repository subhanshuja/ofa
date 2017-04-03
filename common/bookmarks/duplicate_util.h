// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_BOOKMARKS_DUPLICATE_UTIL_H_
#define COMMON_BOOKMARKS_DUPLICATE_UTIL_H_

#include <set>
#include <string>

#include "base/time/time.h"

#include "components/bookmarks/browser/bookmark_node.h"

namespace bookmarks {
class BookmarkNode;
}

namespace opera {
typedef std::string FlawId;

struct EqualBookmarkIDs {
  bool operator()(const bookmarks::BookmarkNode* lhs,
                  const bookmarks::BookmarkNode* rhs) const {
    return (lhs->id() > rhs->id());
  }
};

typedef std::set<const bookmarks::BookmarkNode*, EqualBookmarkIDs>
    BookmarkNodeList;

enum TrackerState {
  TRACKER_STATE_UNSET,
  TRACKER_STATE_IDLE,
  TRACKER_STATE_INDEXING_SCHEDULED,
  TRACKER_STATE_INDEXING,
  TRACKER_STATE_INDEXING_COMPLETE,
  TRACKER_STATE_PROCESSING_SCHEDULED,
  TRACKER_STATE_PROCESSING,
  TRACKER_STATE_WAITING_FOR_SYNC,
  TRACKER_STATE_STOPPED
};

enum TrackerSyncState {
  // The uninitialized state.
  SYNC_STATE_UNKNOWN,
  // Bookmarks datatype is being configured and associated.
  SYNC_STATE_ASSOCIATING,
  // Bookmarks datatype is associated.
  SYNC_STATE_ASSOCIATED,
  // Bookmarks datatype is disassociated or user is logged out.
  SYNC_STATE_DISASSOCIATED,
  // Association error, service error.
  SYNC_STATE_ERROR,
  // Boundary.
  SYNC_STATE_LAST
};

// The reason for last bookmark model scan scheduled by the
// tracker.
enum ScanSource {
  // The uninitialized state.
  SCAN_SOURCE_UNSET,
  // Scan was triggered by the local bookmark model change.
  SCAN_SOURCE_MODEL_CHANGE,
  // Scan is due because of a sync cycle that included bookmark
  // commits from the local model.
  SCAN_SOURCE_SYNC_CYCLE,
  // The initial scan due to the bookmark model loaded event.
  SCAN_SOURCE_MODEL_LOADED,
  // Trigerred with sync state change in order to ensure that the original
  // elector and sync helper changes are followed up.
  SCAN_SOURCE_SYNC_ENABLED,
  SCAN_SOURCE_SYNC_DISABLED,
  // Scan was triggered after extensive model changes were ended.
  SCAN_SOURCE_EXTENSIVE_CHANGES_END,
  // Scan was trigerred at tracker startup.
  SCAN_SOURCE_TRACKER_STARTED,
  // Scan was trigerred due to backoff period end.
  SCAN_SOURCE_BACKOFF_END,
};

enum NodeSyncState { SYNCED, UNSYNCED, UNKNOWN };

enum IdSearchType {
  // Only pick flaws that the original node can be chosen for at the time.
  // Note that while there is no problem with chosing the original at
  // any time with the local original elector, things are different with
  // the sync original elector as some sync nodes might not have been
  // recognized by the server yet.
  HAS_ORIGINAL = 1 << 2,
  // Only pick flaws regarding folders.
  IS_FOLDER = 1 << 4,
  // Only pick flaws regarding bookmrks.
  IS_BOOKMARK = 1 << 8,
};
IdSearchType operator|(IdSearchType a, IdSearchType b);

// Describes a bookmark model change.
// Used to remember changes done to the model during de-duplication
// in order to mark that it is safe to apply the changes to the index
// as they are not coming from any external source (UI, sync, extension).
class BookmarkModelChange {
 public:
  enum Type { TYPE_NULL, TYPE_REMOVE, TYPE_MOVE };

  static BookmarkModelChange NullChange();
  static BookmarkModelChange MoveChange(
      const bookmarks::BookmarkNode* old_parent,
      int old_index,
      const bookmarks::BookmarkNode* new_parent,
      int new_index);
  static BookmarkModelChange RemoveChange(const bookmarks::BookmarkNode* parent,
                                          int index,
                                          const bookmarks::BookmarkNode* node);

  bool Equals(const BookmarkModelChange& other) const;

  BookmarkModelChange(Type type,
                      const bookmarks::BookmarkNode* node1,
                      int index1,
                      const bookmarks::BookmarkNode* node2,
                      int index2);

  Type type() const;
  const bookmarks::BookmarkNode* node1() const;
  int index1() const;
  const bookmarks::BookmarkNode* node2() const;
  int index2() const;

 protected:
  Type type_;
  const bookmarks::BookmarkNode* node1_;
  const bookmarks::BookmarkNode* node2_;
  int index1_;
  int index2_;
};

std::string DescribeBookmark(const bookmarks::BookmarkNode* node);
std::string ScanSourceToString(ScanSource source);
std::string TrackerStateToString(TrackerState state);
std::string TrackerSyncStateToString(TrackerSyncState sync_state);
}  // namespace opera
#endif  // COMMON_BOOKMARKS_DUPLICATE_UTIL_H_
