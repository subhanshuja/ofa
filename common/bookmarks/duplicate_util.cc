// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/bookmarks/duplicate_util.h"

#include "base/logging.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/bookmarks/browser/bookmark_node.h"

namespace opera {

IdSearchType operator|(IdSearchType a, IdSearchType b) {
  return static_cast<IdSearchType>(static_cast<int>(a) | static_cast<int>(b));
}

BookmarkModelChange BookmarkModelChange::NullChange() {
  BookmarkModelChange change(BookmarkModelChange::TYPE_NULL, nullptr, -1,
                             nullptr, -1);
  return change;
}

BookmarkModelChange BookmarkModelChange::MoveChange(
    const bookmarks::BookmarkNode* old_parent,
    int old_index,
    const bookmarks::BookmarkNode* new_parent,
    int new_index) {
  BookmarkModelChange change(BookmarkModelChange::TYPE_MOVE, old_parent,
                             old_index, new_parent, new_index);
  return change;
}

BookmarkModelChange BookmarkModelChange::RemoveChange(
    const bookmarks::BookmarkNode* parent,
    int index,
    const bookmarks::BookmarkNode* node) {
  BookmarkModelChange change(BookmarkModelChange::TYPE_REMOVE, parent, index,
                             node, -1);
  return change;
}

BookmarkModelChange::BookmarkModelChange(Type type,
                                         const bookmarks::BookmarkNode* node1,
                                         int index1,
                                         const bookmarks::BookmarkNode* node2,
                                         int index2)
    : type_(type),
      node1_(node1),
      node2_(node2),
      index1_(index1),
      index2_(index2) {}

BookmarkModelChange::Type BookmarkModelChange::type() const {
  return type_;
}

const bookmarks::BookmarkNode* BookmarkModelChange::node1() const {
  return node1_;
}

int BookmarkModelChange::index1() const {
  return index1_;
}

const bookmarks::BookmarkNode* BookmarkModelChange::node2() const {
  return node2_;
}

int BookmarkModelChange::index2() const {
  return index2_;
}

bool BookmarkModelChange::Equals(const BookmarkModelChange& other) const {
  return other.type() == type() && other.node1() == node1() &&
         other.index1() == index1() && other.node2() == node2() &&
         other.index2() == index2();
}

std::string DescribeBookmark(const bookmarks::BookmarkNode* node) {
  if (!node)
    return "[nullptr]";

  int64_t id = node->id();
  int64_t parent_id = -1;
  if (node->parent())
    parent_id = node->parent()->id();
  std::string title = "EMPTY_TITLE";
  if (!node->GetTitle().empty())
    title = base::UTF16ToUTF8(node->GetTitle());

  std::string url;
  std::string type;
  std::string string_id = base::Int64ToString(id);
  std::string string_parent_id = base::Int64ToString(parent_id);

  if (node->is_url()) {
    url = node->url().spec();
    type = "URL";
  } else {
    type = "FOL";
  }

  return "[" + type + " " + string_id + "(" + string_parent_id + ")\t'" +
         title + "'\t'" + url + "']";
}

std::string ScanSourceToString(ScanSource source) {
  switch (source) {
    case SCAN_SOURCE_UNSET:
      return "UNSET";
    case SCAN_SOURCE_MODEL_CHANGE:
      return "MODEL_CHANGE";
    case SCAN_SOURCE_SYNC_CYCLE:
      return "SYNC_CYCLE";
    case SCAN_SOURCE_MODEL_LOADED:
      return "MODEL_LOADED";
    case SCAN_SOURCE_SYNC_ENABLED:
      return "SYNC_ENABLED";
    case SCAN_SOURCE_SYNC_DISABLED:
      return "SYNC_DISABLED";
    case SCAN_SOURCE_EXTENSIVE_CHANGES_END:
      return "EXTENSIVE_CHANGES_END";
    case SCAN_SOURCE_TRACKER_STARTED:
      return "TRACKER_STARTED";
    case SCAN_SOURCE_BACKOFF_END:
      return "BACKOFF_END";
    default:
      NOTREACHED();
      return "<UNKNOWN>";
  };
}

std::string TrackerStateToString(TrackerState state) {
  switch (state) {
    case TRACKER_STATE_UNSET:
      return "UNSET";
    case TRACKER_STATE_IDLE:
      return "IDLE";
    case TRACKER_STATE_INDEXING_SCHEDULED:
      return "INDEXING_SCHEDULED";
    case TRACKER_STATE_INDEXING:
      return "INDEXING";
    case TRACKER_STATE_INDEXING_COMPLETE:
      return "INDEXING_COMPLETE";
    case TRACKER_STATE_PROCESSING_SCHEDULED:
      return "PROCESSING_SCHEDULED";
    case TRACKER_STATE_PROCESSING:
      return "PROCESSING";
    case TRACKER_STATE_WAITING_FOR_SYNC:
      return "WAITING_FOR_SYNC";
    case TRACKER_STATE_STOPPED:
      return "STOPPED";
    default:
      NOTREACHED() << state;
      return "<UNKNOWN>";
  }
}

std::string TrackerSyncStateToString(TrackerSyncState sync_state) {
  switch (sync_state) {
    case SYNC_STATE_UNKNOWN:
      return "UNKNOWN";
    case SYNC_STATE_ASSOCIATED:
      return "ASSOCIATED";
    case SYNC_STATE_DISASSOCIATED:
      return "DISASSOCIATED";
    case SYNC_STATE_ERROR:
      return "ERROR";
    case SYNC_STATE_ASSOCIATING:
      return "ASSOCIATING";

    default:
      NOTREACHED();
      return "<UNKNOWN>";
  }
}

}  // namespace opera
