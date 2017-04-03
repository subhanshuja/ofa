// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOBILE_COMMON_SYNC_OPERA_VERSION_INFO_H_
#define MOBILE_COMMON_SYNC_OPERA_VERSION_INFO_H_

#include <string>

#include "components/version_info/version_info.h"

namespace opera_version_info {

std::string GetProductName();

// Name plus version to be used in the UA string, e.g.: OPR/1.2.3.4
std::string GetProductNameAndVersionForUserAgent();

// Returns version number in format 30.0.1800.0 or 30.0.1800.0.1 if the fifth
// "build" part is nonzero.
std::string GetVersionNumber();

// Given |channel|, returns a human-readable modifier for the version string.
// Use this function instead of hardcoding "developer", "beta" and "Stable".
std::string ChannelToString(version_info::Channel channel);

// Returns the channel.
// Possible values are Channel::STABLE, Channel::BETA, Channel::DEV.
version_info::Channel GetChannel();

}  // namespace opera_version_info

namespace chrome {

std::string GetChannelString();

version_info::Channel GetChannel();

}  // namespace chrome

#endif  // MOBILE_COMMON_SYNC_OPERA_VERSION_INFO_H_
