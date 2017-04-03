// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_DOWNLOAD_DOWNLOAD_MANAGER_OBSERVER_H_
#define CHILL_BROWSER_DOWNLOAD_DOWNLOAD_MANAGER_OBSERVER_H_

#include <string>

#include "content/public/browser/download_item.h"
#include "content/public/browser/download_manager.h"

namespace opera {

class DownloadManagerObserver : public content::DownloadManager::Observer {
 public:
  // Inherited from DownloadManager::Observer
  virtual void OnDownloadCreated(content::DownloadManager* manager,
                                 content::DownloadItem* item) override;

  // Opera extensions
  virtual void OnDownloadCreated(content::DownloadItem* item,
                                 std::string file_name,
                                 content::DownloadItem::DownloadState state,
                                 bool paused,
                                 int progress) = 0;
};

}  // namespace opera

#endif  // CHILL_BROWSER_DOWNLOAD_DOWNLOAD_MANAGER_OBSERVER_H_
