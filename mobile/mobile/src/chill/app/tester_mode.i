// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA. All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/app/tester_mode.h"
%}

namespace opera {

class TesterMode {
 public:
  static bool Enabled();
  static void SetEnabled(bool enabled);

 private:
  TesterMode();
};

}  // namespace opera
