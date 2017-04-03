// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/common/web_application_info.cc
// @final-synchronized

#include "chill/common/webapps/web_application_info.h"

namespace opera {

WebApplicationInfo::IconInfo::IconInfo() : width(0), height(0) {
}

WebApplicationInfo::IconInfo::~IconInfo() {
}

WebApplicationInfo::WebApplicationInfo()
    : mobile_capable(MOBILE_CAPABLE_UNSPECIFIED),
      generated_icon_color(SK_ColorTRANSPARENT) {
}

WebApplicationInfo::~WebApplicationInfo() {
}

}  // namespace opera
