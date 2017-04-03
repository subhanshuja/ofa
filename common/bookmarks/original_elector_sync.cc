// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/bookmarks/original_elector_sync.h"

#include "base/location.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/sync_bookmarks/bookmark_model_associator.h"
#include "components/sync/core/read_transaction.h"
#include "components/sync/syncable/entry.h"

#include "common/bookmarks/duplicate_tracker.h"

using bookmarks::BookmarkNode;

namespace opera {
OriginalElectorSync::OriginalElectorSync(
    browser_sync::ProfileSyncService* service,
    sync_bookmarks::BookmarkModelAssociator* associator)
    : service_(service), associator_(associator) {
  DCHECK(service_);
  DCHECK(associator_);
}

OriginalElectorSync::~OriginalElectorSync() {}

bool OriginalElectorSync::PrepareElection(BookmarkNodeList list) {
  DCHECK(map_.empty());
  syncer::ReadTransaction trans(FROM_HERE, service_->GetUserShare());
  for (const BookmarkNode* node : list) {
    DCHECK(node);
    syncer::ReadNode sync_node(&trans);
    if (!associator_->InitSyncNodeFromChromeId(node->id(), &sync_node))
      return false;
    syncer::syncable::Id sync_id = sync_node.GetEntry()->GetId();
    if (!sync_id.ServerKnows())
      return false;
    map_[node] = sync_id;
  }

  return true;
}

void OriginalElectorSync::FinishElection() {
  map_.clear();
}

bool OriginalElectorSync::Compare(const bookmarks::BookmarkNode* a,
                                  const bookmarks::BookmarkNode* b) {
  DCHECK(a);
  DCHECK(b);
  auto it_a = map_.find(a);
  auto it_b = map_.find(b);
  DCHECK(it_a != map_.end());
  DCHECK(it_b != map_.end());
  syncer::syncable::Id sync_id_a = it_a->second;
  syncer::syncable::Id sync_id_b = it_b->second;
  return (sync_id_a < sync_id_b);
}
}
