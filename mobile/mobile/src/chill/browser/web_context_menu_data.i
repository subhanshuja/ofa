// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

// Swig blink::WebContextMenuData::MediaType, be sure to not swig anything else.
%rename("$ignore", regextarget=1, fullname=1, notregexmatch$name="MediaType") "blink::WebContextMenuData::*";
%include <third_party/WebKit/public/web/WebContextMenuData.h>
