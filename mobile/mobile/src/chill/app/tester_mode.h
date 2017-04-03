// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_APP_TESTER_MODE_H_
#define CHILL_APP_TESTER_MODE_H_

#include "base/macros.h"

namespace opera {

// Tester mode is a "secret" mode where some urls become accessible. The
// testing mode is enabled per session.

class TesterMode {
 public:
  static bool Enabled();
  static void SetEnabled(bool enabled);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(TesterMode);
};

}  // namespace opera

#endif  // CHILL_APP_TESTER_MODE_H_
