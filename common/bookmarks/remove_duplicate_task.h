// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_BOOKMARKS_DUPLICATE_TASK_REMOVE_DUPLICATE_H_
#define COMMON_BOOKMARKS_DUPLICATE_TASK_REMOVE_DUPLICATE_H_

#include <string>

#include "common/bookmarks/duplicate_task.h"

namespace bookmarks {
class BookmarkNode;
}  // namespace bookmarks

namespace opera {
class DuplicateRemover;

// A DuplicateTask implementation that given a bookmark flaw original
// and one of its duplicates will ensure that the duplicate is removed
// from the model. The task does not expect any changes to the model
// other than the ones made by the task itself, in case the task continues
// to run over the model after such changes, its behaviour is undefined.
class RemoveDuplicateTask : public DuplicateTask {
 public:
  RemoveDuplicateTask(DuplicateTracker* tracker,
                               const bookmarks::BookmarkNode* original,
                               const bookmarks::BookmarkNode* duplicate);

  void RunImpl() override;
  bool IsFinished() override;
  std::string name() const override;

 private:
  ~RemoveDuplicateTask() override;

  // Helper methods that make changes to the bookmark model
  // along with notifying the tracker about expected model changes
  // in order to avoid rescans and remembering the unsynced node IDs
  // in order for the next RunImpl() to be able to wait for sync.
  void DoRemoveItem(const bookmarks::BookmarkNode* item);
  void DoMoveItem(const bookmarks::BookmarkNode* item,
                  const bookmarks::BookmarkNode* dest);

  // Moves the item to trash, if available. Removes it from the model
  // otherwise.
  void RemoveOrTrashItem(const bookmarks::BookmarkNode* item);

  const bookmarks::BookmarkNode* original_;
  const bookmarks::BookmarkNode* duplicate_;
  DuplicateTracker* tracker_;
};
}  // namespace opera
#endif  // COMMON_BOOKMARKS_DUPLICATE_TASK_REMOVE_DUPLICATE_H_
