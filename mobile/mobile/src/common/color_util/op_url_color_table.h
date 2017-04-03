// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_ANDROID_SRC_COMMON_COLORUTIL_COLORUTIL_H_
#define MOBILE_ANDROID_SRC_COMMON_COLORUTIL_COLORUTIL_H_

#include <string>

#include "mobile/common/url_color_util/url_color_table.h"

class GURL;

class OpURLColorTable {
 public:
  struct ColorResult {
    uint32_t foreground_color;
    uint32_t background_color;
  };

  OpURLColorTable(const char* data, int length);

  ColorResult LookupColorForUrl(const GURL& url);

 private:
  mobile::URLColorTable url_color_table_;
};

#endif  // MOBILE_ANDROID_SRC_COMMON_COLORUTIL_COLORUTIL_H_
