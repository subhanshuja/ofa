// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_DOWNLOAD_DOWNLOAD_ITEM_OBSERVER_H_
#define CHILL_BROWSER_DOWNLOAD_DOWNLOAD_ITEM_OBSERVER_H_

#include <stdint.h>

#include "content/public/browser/download_item.h"

namespace opera {

class DownloadItemObserver : public content::DownloadItem::Observer {
 public:
  DownloadItemObserver();

  // Inherited from DownloadItem::Observer
  virtual void OnDownloadUpdated(content::DownloadItem* download);

  // Opera extensions.
  virtual void OnUpdated(int64_t received_bytes, int64_t total_bytes,
                         int64_t rate) = 0;
  virtual void OnStateChanged(content::DownloadItem::DownloadState state,
                              bool paused) = 0;

 private:
  // Keep a cache of the currently known state of the DownloadItem for
  // comparison in OnDownloadUpdated. This way we can call up to java only on
  // when there's a change.
  bool is_paused_;
  content::DownloadItem::DownloadState state_;
  int64_t total_bytes_;
  int64_t received_bytes_;
  int64_t rate_;
};

}  // namespace opera

#endif  // CHILL_BROWSER_DOWNLOAD_DOWNLOAD_ITEM_OBSERVER_H_
