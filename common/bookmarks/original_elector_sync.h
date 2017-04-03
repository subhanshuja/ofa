// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_BOOKMARKS_DUPLICATE_MODEL_HELPER_SYNC_H_
#define COMMON_BOOKMARKS_DUPLICATE_MODEL_HELPER_SYNC_H_

#include <map>

#include "common/bookmarks/original_elector.h"

#include "components/browser_sync/profile_sync_service.h"

namespace sync_bookmarks {
class BookmarkModelAssociator;
}  // namespace sync_bookmarks

namespace syncer {
namespace syncable {
class Id;
}
}

namespace opera {
// DuplicateModelHelper implementation for the sync-enabled scenario.
// The property used for bookmark node comparision is the server sync node ID.
// Note that this means that the original cannot be elected in case any of the
// nodes passed to the ElectOriginal() method was not already synced to the
// server (to be exact, the local sync nodes have to have an ID that was
// returned by the server after the items have been committed - we'll get
// such an ID momentarily after a GetUpdates response if the item was created
// by another client and syncing down here or after a moment in case the item
// has been created locally from this client's point of view and is pending a
// commit and a commit response).
class OriginalElectorSync : public OriginalElector {
 public:
  OriginalElectorSync(browser_sync::ProfileSyncService* service,
                      sync_bookmarks::BookmarkModelAssociator* associator);
  ~OriginalElectorSync() override;

  const bookmarks::BookmarkNode* ElectOriginal(BookmarkNodeList list);

 protected:
  bool PrepareElection(BookmarkNodeList list) override;
  void FinishElection() override;
  bool Compare(const bookmarks::BookmarkNode* a,
               const bookmarks::BookmarkNode* b) override;

  std::map<const bookmarks::BookmarkNode*, syncer::syncable::Id> map_;
  browser_sync::ProfileSyncService* service_;
  sync_bookmarks::BookmarkModelAssociator* associator_;
};
}  // namespace opera

#endif  // COMMON_BOOKMARKS_DUPLICATE_ELECTOR_SYNC_H_
