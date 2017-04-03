// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "common/color_util/op_url_color_table.h"
%}

%include "various.i"

%nodefaultctor OpURLColorTable;
%nodefaultdtor OpURLColorTable;

class OpURLColorTable {
 public:
  struct ColorResult {
    uint32_t foreground_color;
    uint32_t background_color;
  };

  OpURLColorTable(char* BYTE, int length);

  ColorResult LookupColorForUrl(const GURL& url);
};
