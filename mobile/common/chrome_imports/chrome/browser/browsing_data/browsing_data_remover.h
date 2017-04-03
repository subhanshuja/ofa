// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_REMOVER_H_
#define CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_REMOVER_H_

#include "base/sequenced_task_runner_helpers.h"
#include "base/time/time.h"
#include "chrome/common/features.h"
#include "components/keyed_service/core/keyed_service.h"

namespace content {
class BrowserContext;
class PluginDataRemover;
class StoragePartition;
}

class BrowsingDataRemover : public KeyedService {
 public:
  enum RemoveDataMask {
    REMOVE_ALL,
  };

  struct TimeRange {
    TimeRange(base::Time begin, base::Time end) : begin(begin), end(end) {}

    base::Time begin;
    base::Time end;
  };

  // Observer is notified when the removal is done. Done means keywords have
  // been deleted, cache cleared and all other tasks scheduled.
  class Observer {
   public:
    virtual void OnBrowsingDataRemoverDone() = 0;

   protected:
    virtual ~Observer() {}
  };

  // Removes the specified items related to browsing for all origins that match
  // the provided |origin_type_mask| (see BrowsingDataHelper::OriginTypeMask).
  void Remove(const TimeRange& time_range,
              int remove_mask,
              int origin_type_mask);

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 private:
  friend class BrowsingDataRemoverFactory;

  BrowsingDataRemover(content::BrowserContext* browser_context);
  ~BrowsingDataRemover() override;

  DISALLOW_COPY_AND_ASSIGN(BrowsingDataRemover);
};

#endif  // CHROME_BROWSER_BROWSING_DATA_BROWSING_DATA_REMOVER_H_
