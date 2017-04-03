// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/url_fixer_upper/url_fixer_upper.h"

#include "components/url_formatter/url_fixer.h"

std::string OpURLFixerUpper::FixupURL(const std::string& text,
                                      const std::string& desired_tld) {
  return url_formatter::FixupURL(text, desired_tld).spec();
}
