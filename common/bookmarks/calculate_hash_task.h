// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_BOOKMARKS_DUPLICATE_TASK_CALCULATE_HASH_H_
#define COMMON_BOOKMARKS_DUPLICATE_TASK_CALCULATE_HASH_H_

#include <string>

#include "common/bookmarks/duplicate_task.h"

namespace bookmarks {
class BookmarkModel;
class BookmarkNode;
}  // namespace bookmarks

namespace opera {
class DuplicateTracker;

// Given a bookmark node, ensure that the node and all of its children
// are seen by the tracker. Should not be given any ignored nodes.
// The task will call the DuplicateTracker::AppendNodeToMap() method for
// each node it goes over. Given an URL node, the task will add the node
// to the tracker's map and finish immediately. Given a folder node, the task
// will post a CalculateHashTask for each of its children and add the folder's
// node to the tracker's map.
// The task is not expecting any changes in the node it iterates over,
// especially not in the child count/order. In case any model changes regarding
// the iterated node are detected, the task MUST be cancelled or its behaviour
// is undefined.
class CalculateHashTask : public DuplicateTask {
 public:
  CalculateHashTask(DuplicateTracker* tracker,
                    const bookmarks::BookmarkNode* node);

  void RunImpl() override;
  bool IsFinished() override;
  std::string name() const override;

 protected:
  ~CalculateHashTask() override;

  void DoCalculateHash(const bookmarks::BookmarkNode* node);

  // The current child index.
  int index_;
  bool finished_;
  const bookmarks::BookmarkNode* node_;
  DuplicateTracker* tracker_;
};
}  // namespace opera
#endif  // COMMON_BOOKMARKS_DUPLICATE_TASK_CALCULATE_HASH_H_
