// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_STATE_STORE_H_
#define COMMON_SYNC_SYNC_STATE_STORE_H_

#include <string>

#include "common/sync/sync_data.h"

namespace opera {

/**
 * An abstraction of a store of sync states for each data type.
 *
 * In production code, it should be implemented as a persistent store so that
 * the sync states are preserved across restarts.
 */
class SyncStateStore {
 public:
  virtual ~SyncStateStore() {}

  /** @return @c true if the store contains the sync state for @p data_type */
  virtual bool HasSyncState(const SyncDataType& data_type) const = 0;

  /**
   * Gets the sync state for a data type.  The behavior is undefined if the
   * store doesn't contain the sync state for that data type.
   */
  virtual std::string GetSyncState(const SyncDataType& data_type) const = 0;

  /**
   * Sets the sync state for a data type in the store.  Overwrites any value
   * stored previously for that data type.
   */
  virtual void SetSyncState(const SyncDataType& data_type,
                            const std::string& sync_state) = 0;

  /**
   * Clears the sync state for given datatype.
   */
  virtual void ClearSyncState(const SyncDataType& data_type) = 0;
};

}  // namespace opera

#endif  // COMMON_SYNC_SYNC_STATE_STORE_H_
