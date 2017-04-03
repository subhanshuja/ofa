// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/bookmarks/remove_duplicate_task.h"

#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"

using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;

namespace opera {
RemoveDuplicateTask::RemoveDuplicateTask(DuplicateTracker* tracker,
                                         const BookmarkNode* original,
                                         const BookmarkNode* duplicate)
    : original_(original), duplicate_(duplicate), tracker_(tracker) {
  DCHECK(original_ && duplicate_ && tracker_);
}

RemoveDuplicateTask::~RemoveDuplicateTask() {}

void RemoveDuplicateTask::RunImpl() {
  DCHECK(tracker_->IsSyncState(SYNC_STATE_ASSOCIATED) ||
         tracker_->IsSyncState(SYNC_STATE_DISASSOCIATED));
  DCHECK(!tracker_->ShouldWaitForSync());
  DCHECK(duplicate_);

  if (duplicate_->is_folder()) {
    if (duplicate_->child_count() == 0) {
      RemoveOrTrashItem(duplicate_);
      duplicate_ = nullptr;
    } else {
      const BookmarkNode* current = duplicate_->GetChild(0);
      DCHECK(current);
      const BookmarkNode* counterpart =
          tracker_->FindCounterpart(current, original_);
      if (counterpart) {
        DCHECK(current->is_folder() || current->is_url());
        if (current->is_url()) {
          RemoveOrTrashItem(current);
          current = nullptr;
        } else if (current->is_folder()) {
          tracker_->PostDeduplicationTask(counterpart, current);
        }
      } else {
        DoMoveItem(current, original_);
      }
    }
  } else if (duplicate_->is_url()) {
    RemoveOrTrashItem(duplicate_);
    duplicate_ = nullptr;
  }
}

bool RemoveDuplicateTask::IsFinished() {
  return duplicate_ == nullptr;
}

std::string RemoveDuplicateTask::name() const {
  std::string name;
  name = "RemoveDuplicateTask [" + DescribeBookmark(original_) + "] <- [" +
         DescribeBookmark(duplicate_) + "]";
  return name;
}

void RemoveDuplicateTask::DoRemoveItem(const bookmarks::BookmarkNode* item) {
  DCHECK(item && item->parent());
  if (tracker_->IsSyncState(SYNC_STATE_ASSOCIATED))
    tracker_->AddNodeToWaitForSync(item);
  tracker_->SetExpectedChange(BookmarkModelChange::RemoveChange(
      item->parent(), item->parent()->GetIndexOf(item), item));
  tracker_->bookmark_model()->Remove(item);
}

void RemoveDuplicateTask::DoMoveItem(const BookmarkNode* item,
                                     const BookmarkNode* dest) {
  DCHECK(dest);
  DCHECK(item && item->parent());
  if (tracker_->IsSyncState(SYNC_STATE_ASSOCIATED))
    tracker_->AddNodeToWaitForSync(item);
  tracker_->SetExpectedChange(BookmarkModelChange::MoveChange(
      item->parent(), item->parent()->GetIndexOf(item), dest, 0));
  tracker_->bookmark_model()->Move(item, dest, 0);
}

void RemoveDuplicateTask::RemoveOrTrashItem(
    const bookmarks::BookmarkNode* item) {
  if (tracker_->ShouldRemoveDuplicatesFromModel()) {
    DoRemoveItem(item);
  } else {
    DCHECK(tracker_->trash_node());
    DoMoveItem(item, tracker_->trash_node());
  }
}

}  // namespace opera
