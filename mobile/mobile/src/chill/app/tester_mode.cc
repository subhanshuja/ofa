// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/app/tester_mode.h"

namespace {

bool g_tester_mode_enabled = 0;

}  // namespace

namespace opera {

bool TesterMode::Enabled() {
  return g_tester_mode_enabled;
}

void TesterMode::SetEnabled(bool enabled) {
  g_tester_mode_enabled = enabled;
}

}  // namespace opera
