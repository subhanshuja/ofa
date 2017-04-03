// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/color_util/op_url_color_table.h"

#include <stdint.h>

#include "url/gurl.h"

OpURLColorTable::OpURLColorTable(const char* data, int length)
    : url_color_table_(reinterpret_cast<const uint8_t*>(data), length) {}

OpURLColorTable::ColorResult OpURLColorTable::LookupColorForUrl(
    const GURL& url) {
  uint32_t foreground_color = 0, background_color = 0, invert = 0;
  url_color_table_.LookupColorForUrl(url, &foreground_color, &background_color,
    &invert);
  return {foreground_color, background_color};
}
