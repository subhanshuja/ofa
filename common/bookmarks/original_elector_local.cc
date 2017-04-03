// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/bookmarks/original_elector_local.h"

#include "components/bookmarks/browser/bookmark_node.h"

using bookmarks::BookmarkNode;

namespace opera {
OriginalElectorLocal::OriginalElectorLocal() {}

OriginalElectorLocal::~OriginalElectorLocal() {}

bool OriginalElectorLocal::PrepareElection(BookmarkNodeList list) {
  return true;
}

void OriginalElectorLocal::FinishElection() {
  // Empty.
}

bool OriginalElectorLocal::Compare(const bookmarks::BookmarkNode* a,
                                   const bookmarks::BookmarkNode* b) {
  DCHECK(a);
  DCHECK(b);
  return a->id() < b->id();
}
}  // namespace opera
