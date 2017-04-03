// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/download/opera_download_manager_delegate.h"
%}

%feature("director", assumeoverride=1) opera::OperaDownloadManagerDelegate;

SWIG_SELFREF_NAMESPACED_CONSTRUCTOR(opera, OperaDownloadManagerDelegate);

namespace opera {

class OperaDownloadManagerDelegate {
 public:
  OperaDownloadManagerDelegate(bool store_downloads);
  virtual ~OperaDownloadManagerDelegate();

  virtual void OnShowDownloadPathDialog(
      unsigned int download_id,
      OpRunnable callback,
      const base::string16& suggested_filename) = 0;
};

}  // namespace opera
