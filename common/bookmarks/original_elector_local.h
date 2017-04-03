// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_BOOKMARKS_DUPLICATE_ELECTOR_DEFAULT_H_
#define COMMON_BOOKMARKS_DUPLICATE_ELECTOR_DEFAULT_H_

#include "common/bookmarks/original_elector.h"

namespace opera {
// A DuplicateModelHelper implementation for a sync-disabled scenario.
// Election of original is done basing on bookmark node ID, as the value
// is not expected to change during a browser session.
// The sync-related methods are not implemented and return the default
// values found in the base class.
class OriginalElectorLocal : public OriginalElector {
 public:
  OriginalElectorLocal();
  ~OriginalElectorLocal() override;

  const bookmarks::BookmarkNode* ElectOriginal(BookmarkNodeList list);

 protected:
  bool PrepareElection(BookmarkNodeList list) override;
  void FinishElection() override;
  bool Compare(const bookmarks::BookmarkNode* a,
               const bookmarks::BookmarkNode* b) override;
};
}

#endif  // COMMON_BOOKMARKS_DUPLICATE_ELECTOR_DEFAULT_H_
