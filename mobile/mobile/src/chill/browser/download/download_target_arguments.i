// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/download/download_target_arguments.h"
%}

namespace opera {

class DownloadTargetArguments : public OpArguments {
public:
  std::string targetPath;
};

}  // namespace opera
