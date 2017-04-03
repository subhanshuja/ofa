// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_DOWNLOAD_DOWNLOAD_TARGET_ARGUMENTS_H_
#define CHILL_BROWSER_DOWNLOAD_DOWNLOAD_TARGET_ARGUMENTS_H_

#include <string>

namespace opera {

class DownloadTargetArguments : public OpArguments {
 public:
  std::string targetPath;
 private:
  SWIG_CLASS_NAME
};

}  // namespace opera

#endif  // CHILL_BROWSER_DOWNLOAD_DOWNLOAD_TARGET_ARGUMENTS_H_
