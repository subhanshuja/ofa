// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/download/download_item_observer.h"
%}

namespace opera {

%feature("director", assumeoverride=1) DownloadItemObserver;

class DownloadItemObserver {
public:
  virtual ~DownloadItemObserver();

  // Opera extensions
  virtual void OnUpdated(int64_t received_bytes, int64_t total_bytes, int64_t rate) = 0;
  virtual void OnStateChanged(content::DownloadItem::DownloadState state, bool paused) = 0;
};

}  // namespace opera
