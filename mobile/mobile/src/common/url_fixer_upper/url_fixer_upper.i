// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "common/url_fixer_upper/url_fixer_upper.h"
%}

class OpURLFixerUpper {
 public:
  static std::string FixupURL(const std::string& text,
                              const std::string& desired_tld);
};
