// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/net/tld_validity.h"
%}

namespace opera {

class TldValidity {
 public:
  static bool HasValidTLD(const GURL &);
};

}  // namespace opera
