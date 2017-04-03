// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/download/download_item_observer.h"

#include <string>

namespace opera {

DownloadItemObserver::DownloadItemObserver()
    : is_paused_(false)
    , state_(content::DownloadItem::MAX_DOWNLOAD_STATE), total_bytes_(-1)
    , received_bytes_(-1) {
}

void DownloadItemObserver::OnDownloadUpdated(content::DownloadItem* download) {
  bool currently_paused = download->IsPaused();
  content::DownloadItem::DownloadState current_state = download->GetState();

  if (currently_paused != is_paused_ || current_state != state_) {
    is_paused_ = currently_paused;
    state_ = current_state;
    OnStateChanged(state_, is_paused_);
  }

  if (received_bytes_ != download->GetReceivedBytes() ||
      total_bytes_ != download->GetTotalBytes() ||
      rate_ != download->CurrentSpeed()) {
    received_bytes_ = download->GetReceivedBytes();
    total_bytes_ = download->GetTotalBytes();
    rate_ = download->CurrentSpeed();
    OnUpdated(received_bytes_, total_bytes_, rate_);
  }
}

}  // namespace opera
