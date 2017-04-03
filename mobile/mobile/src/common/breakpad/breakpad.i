// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "common/breakpad/breakpad_reporter.h"
%}


%feature("director") opera::BreakpadReporter;
%rename("NativeBreakpadReporter") opera::BreakpadReporter;

namespace opera {

class BreakpadReporter {
 public:
  virtual void UploadDumps() = 0;

  static void SetCrashDumpsDir(const std::string& dir);

  virtual ~BreakpadReporter();
};

}  // namespace opera
