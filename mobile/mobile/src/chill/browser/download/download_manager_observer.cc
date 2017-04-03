// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/download/download_manager_observer.h"

namespace opera {

void DownloadManagerObserver::OnDownloadCreated(
    content::DownloadManager* manager, content::DownloadItem* item) {

  if (!item->IsSavePackageDownload()) {
    OnDownloadCreated(item, item->GetTargetFilePath().value(),
                      item->GetState(), item->IsPaused(),
                      item->PercentComplete());
  }
}

}  // namespace opera
