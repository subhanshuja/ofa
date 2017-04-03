// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "content/public/browser/download_manager.h"
%}

namespace content {

class DownloadManager {
public:
  void AddObserver(opera::DownloadManagerObserver* observer);
  void RemoveObserver(opera::DownloadManagerObserver* observer);

private:
  DownloadManager();
  ~DownloadManager();
};

}
