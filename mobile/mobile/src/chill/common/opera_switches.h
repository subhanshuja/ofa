// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/shell_switches.h
// @final-synchronized

// Defines all the opera mobile command-line switches.

#ifndef CHILL_COMMON_OPERA_SWITCHES_H_
#define CHILL_COMMON_OPERA_SWITCHES_H_

namespace switches {

extern const char kEnableRemoteDebugging[];
extern const char kNetLogLevel[];
extern const char kUserAgent[];
extern const char kBypassAppBannerEngagementChecks[];
// Duplicated in src/com/opera/android/BrowserSwitches.java as
// CUSTOM_ARTICLE_PAGE_SERVER
extern const char kCustomArticleTranscodingServer[];
extern const char kDisableTileCompression[];

}  // namespace switches

#endif  // CHILL_COMMON_OPERA_SWITCHES_H_
