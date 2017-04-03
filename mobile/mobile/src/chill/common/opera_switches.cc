// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/shell_switches.cc
// @final-synchronized

#include "chill/common/opera_switches.h"

#include "base/command_line.h"

namespace switches {

// Sets the base logging level for the net log. Log 0 logs the most data.
// Intended primarily for use with --log-net-log.
const char kNetLogLevel[] = "net-log-level";

// A string used to override the default user agent with a custom one.
const char kUserAgent[] = "user-agent";

// This flag causes the user engagement checks for showing app banners to be
// bypassed. It is intended to be used by developers who wish to test that their
// sites otherwise meet the criteria needed to show app banners.
const char kBypassAppBannerEngagementChecks[] =
    "bypass-app-banner-engagement-checks";

// Set a custom article page server, overriding the default.
// Expects a switch value.
const char kCustomArticleTranscodingServer[] = "custom-article-page-server";

// Disable tile textures compression.
const char kDisableTileCompression[] = "disable-tile-compression";

}  // namespace switches
