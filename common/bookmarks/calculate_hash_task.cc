// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/bookmarks/calculate_hash_task.h"

#include "components/bookmarks/browser/bookmark_node.h"

#include "common/bookmarks/duplicate_tracker.h"

using bookmarks::BookmarkNode;

namespace opera {
CalculateHashTask::CalculateHashTask(DuplicateTracker* tracker,
                                     const bookmarks::BookmarkNode* node)
    : index_(0), finished_(false), node_(node), tracker_(tracker) {
  CHECK(node);
  DCHECK(tracker_);
}

CalculateHashTask::~CalculateHashTask() {}

void CalculateHashTask::RunImpl() {
  DCHECK(node_);

  if (node_->is_url()) {
    DoCalculateHash(node_);
    finished_ = true;
  } else if (node_->is_folder()) {
    int child_count = node_->child_count();
    if (index_ < child_count) {
      const BookmarkNode* child = node_->GetChild(index_);
      DCHECK(child);
      tracker_->PostCalculationTask(child);
      index_++;
    } else if (child_count == 0 || index_ == child_count) {
      DoCalculateHash(node_);
      finished_ = true;
    } else {
      NOTREACHED();
    }
  }
}

bool CalculateHashTask::IsFinished() {
  return finished_;
}

std::string CalculateHashTask::name() const {
  std::string name;
  name = "CalculateHashTask [" + DescribeBookmark(node_) + "]";
  return name;
}

void CalculateHashTask::DoCalculateHash(const BookmarkNode* node) {
  tracker_->AppendNodeToMap(node);
}
}  // namespace opera
