// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_URL_FIXER_UPPER_URL_FIXER_UPPER_H_
#define COMMON_URL_FIXER_UPPER_URL_FIXER_UPPER_H_

#include <string>

// Note: We fake a namespace here to make things easier for SWIG (we could use
// a C++ namespace, but then we'd have to do more work in the SWIG system,
// and the result would still be the same: a Java class with public static
// methods).
class OpURLFixerUpper {
 public:
  // @see: url_formatter::FixupURL
  static std::string FixupURL(const std::string& text,
                              const std::string& desired_tld);
};

#endif  // COMMON_URL_FIXER_UPPER_URL_FIXER_UPPER_H_
