// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_DOWNLOAD_DOWNLOAD_HELPER_H_
#define CHILL_BROWSER_DOWNLOAD_DOWNLOAD_HELPER_H_

#include <string>

#include "content/public/browser/download_manager.h"

#include "chill/browser/download/opera_download_manager_delegate.h"

namespace opera {

class DownloadReadFromDiskObserver {
 public:
  virtual ~DownloadReadFromDiskObserver() {}
  virtual void Ready() = 0;
};

class DownloadHelper {
 public:
  static void ReadDownloadsFromDisk(content::DownloadManager* manager,
                                    DownloadReadFromDiskObserver* observer) {
    static_cast<OperaDownloadManagerDelegate*>(manager->GetDelegate())
        ->ReadDownloadMetadataFromDisk(observer);
  }

  static void MigrateDownload(content::DownloadManager* manager,
      std::string filename, std::string url, std::string mimetype,
      int downloaded, int total, bool completed) {
    static_cast<OperaDownloadManagerDelegate*>(manager->GetDelegate())
        ->MigrateDownload(filename,
                          url,
                          mimetype,
                          downloaded,
                          total,
                          completed);
  }
};

}  // namespace opera

#endif  // CHILL_BROWSER_DOWNLOAD_DOWNLOAD_HELPER_H_
