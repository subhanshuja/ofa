// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/download/download_manager_observer.h"
%}

namespace opera {

%feature("director", assumeoverride=1) DownloadManagerObserver;

class DownloadManagerObserver {
public:
  virtual ~DownloadManagerObserver();

  // Opera extensions
  virtual void OnDownloadCreated(content::DownloadItem* item,
                                 std::string file_name,
                                 content::DownloadItem::DownloadState state,
                                 bool paused,
                                 int progress) = 0;
};

}  // namespace opera
