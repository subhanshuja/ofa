// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_BOOKMARK_TRACKER_OBSERVER_H_
#define COMMON_BOOKMARK_TRACKER_OBSERVER_H_

#include "common/bookmarks/duplicate_util.h"

namespace opera {

class DuplicateTracker;

class TrackerObserver {
 public:
  virtual ~TrackerObserver() {}

  virtual void OnTrackerStateChanged(DuplicateTracker* tracker) = 0;
  virtual void OnTrackerStatisticsUpdated(DuplicateTracker* tracker) = 0;
  virtual void OnDuplicateAppeared(DuplicateTracker* tracker, FlawId id) = 0;
  virtual void OnFlawAppeared(DuplicateTracker* tracker, FlawId id) = 0;
  virtual void OnDuplicateDisappeared(DuplicateTracker* tracker, FlawId id) = 0;
  virtual void OnFlawDisappeared(DuplicateTracker* tracker, FlawId id) = 0;
  virtual void OnFlawProcessingStarted(DuplicateTracker* tracker,
                                       FlawId id) = 0;
  virtual void OnDebugStatsUpdated(DuplicateTracker* tracker) = 0;
};
}  // namespace opera
#endif  // COMMON_BOOKMARK_TRACKER_OBSERVER_H_
