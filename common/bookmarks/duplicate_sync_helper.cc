// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/bookmarks/duplicate_sync_helper.h"

#include "components/browser_sync/profile_sync_service.h"

namespace opera {
DuplicateSyncHelper::DuplicateSyncHelper(
    browser_sync::ProfileSyncService* service,
    sync_bookmarks::BookmarkModelAssociator* associator) {
  associator_ = associator;
  service_ = service;
  DCHECK(associator_);
  DCHECK(service_);
}

int64_t DuplicateSyncHelper::GetSyncIdForBookmarkId(int64_t id) {
  int64_t sync_id = associator_->GetSyncIdFromChromeId(id);

  syncer::ReadTransaction trans(FROM_HERE, service_->GetUserShare());
  syncer::ReadNode sync_node(&trans);
  syncer::BaseNode::InitByLookupResult result =
      sync_node.InitByIdLookup(sync_id);
  if (result == syncer::BaseNode::INIT_OK)
    return sync_id;

  return syncer::kInvalidId;
}

NodeSyncState DuplicateSyncHelper::GetNodeSyncState(int64_t id) {
  DCHECK_NE(id, syncer::kInvalidId);

  syncer::ReadTransaction trans(FROM_HERE, service_->GetUserShare());
  syncer::ReadNode sync_node(&trans);
  syncer::BaseNode::InitByLookupResult result = sync_node.InitByIdLookup(id);
  if (result == syncer::BaseNode::INIT_OK ||
      result == syncer::BaseNode::INIT_FAILED_ENTRY_IS_DEL) {
    if (sync_node.GetEntry()->GetIsUnsynced()) {
      return UNSYNCED;
    } else {
      return SYNCED;
    }
  } else {
    return UNKNOWN;
  }
}
}  // namespace opera
