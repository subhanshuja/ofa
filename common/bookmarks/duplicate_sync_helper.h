// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_BOOKMARKS_DUPLICATE_SYNC_HELPER_H_
#define COMMON_BOOKMARKS_DUPLICATE_SYNC_HELPER_H_

#include "components/sync_bookmarks/bookmark_model_associator.h"
#include "components/sync/core/read_transaction.h"
#include "components/sync/syncable/entry.h"

#include "common/bookmarks/duplicate_util.h"

namespace browser_sync {
class ProfileSyncService;
}

namespace opera {
class DuplicateSyncHelper {
 public:
  DuplicateSyncHelper(browser_sync::ProfileSyncService* service,
                      sync_bookmarks::BookmarkModelAssociator* associator);
  virtual ~DuplicateSyncHelper() {}

  virtual int64_t GetSyncIdForBookmarkId(int64_t id);

  // The method only makes sense for the sync helper type.
  // The method will check the sync model for the given sync node ID
  // in order to determine whether the node is marked as pending
  // synchronization or not.
  // |id| The sync model ID returned by the GetSyncIdForBookmarkId()
  //      method.
  // Returns:
  //    NodeSyncState::SYNCED - The node is synced.
  //    NodeSyncState::UNSYNCED - The node is unsynced.
  //    NodeSyncState::UNKNOWN - The check result is not known now.
  virtual NodeSyncState GetNodeSyncState(int64_t id);

 protected:
  browser_sync::ProfileSyncService* service_;
  sync_bookmarks::BookmarkModelAssociator* associator_;
};
}  // namespace opera
#endif  // COMMON_BOOKMARKS_DUPLICATE_SYNC_HELPER_H_
