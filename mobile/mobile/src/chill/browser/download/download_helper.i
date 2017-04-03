// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/download/download_helper.h"
%}

namespace opera {

%feature("director", assumeoverride=1) DownloadReadFromDiskObserver;

class DownloadReadFromDiskObserver {
public:
  virtual ~DownloadReadFromDiskObserver();
  virtual void Ready() = 0;
};

class DownloadHelper {
public:
  static void ReadDownloadsFromDisk(content::DownloadManager* manager,
      DownloadReadFromDiskObserver* observer);
  static void MigrateDownload(content::DownloadManager* manager,
      std::string filename,
      std::string url,
      std::string mimetype,
      int downloaded,
      int total,
      bool completed);

private:
  DownloadHelper();
  ~DownloadHelper();
};

}  // namespace opera
