// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/bookmarks/original_elector.h"

using bookmarks::BookmarkNode;

namespace opera {
OriginalElector::OriginalElector() {}

OriginalElector::~OriginalElector() {}

const bookmarks::BookmarkNode* OriginalElector::ElectOriginal(
    BookmarkNodeList list) {
  DCHECK(list.size() > 1);
  if (!PrepareElection(list)) {
    FinishElection();
    return nullptr;
  }

  const bookmarks::BookmarkNode* original = nullptr;
  for (const BookmarkNode* node : list) {
    if (!original) {
      original = node;
    } else {
      if (Compare(original, node))
        original = node;
    }
  }
  FinishElection();

  return original;
}
}  // namespace opera
