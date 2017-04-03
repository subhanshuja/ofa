// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_BOOKMARKS_DUPLICATE_MODEL_HELPER_H_
#define COMMON_BOOKMARKS_DUPLICATE_MODEL_HELPER_H_

#include "common/bookmarks/duplicate_util.h"

namespace bookmarks {
class BookmarkModel;
class BookmarkNode;
}

namespace opera {
class OriginalElector {
 public:
  OriginalElector();
  virtual ~OriginalElector();

  // Given the duplicate bookmark node list, elect an original in such
  // a way that the result is:
  // a) same each run provided there were no model changes (for the local
  //    elector type);
  // b) same on each sync client for a given account, provided there were
  //    no remote account changes (for the sync elector type).
  // Note that the method does not actually verify that the given
  // bookmark node list actually is a list of duplicates, i.e. that
  // all of the nodes it does contain resolve to the same flaw id.
  // The method returns the elected original node or a nullptr in case
  // the original could not be elected at the given time (possible
  // for the sync helper in case the list contains bookmarks that
  // were not yet confirmed as known by the server).
  // The returned original node will the the one that comes up as the
  // "least" after sorting all the nodes using the Compare() method.
  const bookmarks::BookmarkNode* ElectOriginal(BookmarkNodeList list);

 protected:
  // Run just before the election process is run, should prepare any data
  // structures that will be used during election by the given helper
  // implementation.
  // |list| The bookmark node list passed to the ElectOriginal()
  //       method.
  virtual bool PrepareElection(BookmarkNodeList list) = 0;

  // Run just after the election has finished, should clean up after
  // the PrepareElection() method.
  virtual void FinishElection() = 0;

  // Do compare two given bookmark nodes using any criteria for the
  // given helper type. Return true in case node |a| should be
  // considered "less than" node |b|. Note that the assumption here is
  // that the election bases on some kind of an unique ID, i.e. no
  // two bookmark nodes in the given bookmark node list may have identical
  // IDs comparision-wise.
  virtual bool Compare(const bookmarks::BookmarkNode* a,
                       const bookmarks::BookmarkNode* b) = 0;
};
}  // namespace opera
#endif  // COMMON_BOOKMARKS_DUPLICATE_MODEL_HELPER_H_
