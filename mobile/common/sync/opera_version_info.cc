// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/sync/opera_version_info.h"

#include <base/logging.h>
#include <components/version_info/version_info.h>

namespace opera_version_info {

// static
std::string GetVersionNumber() {
  return version_info::GetVersionNumber();
}

// static
std::string ChannelToString(version_info::Channel channel) {
  return version_info::GetChannelString(channel);
}

// static
version_info::Channel GetChannel() {
#ifdef OPERA_DEVELOPER_BUILD
  return version_info::Channel::DEV;
#else
#ifdef OPERA_BETA_BUILD
  return version_info::Channel::BETA;
#else
  return version_info::Channel::STABLE;
#endif
#endif
}

}  // namespace version_info

namespace chrome {

//static
std::string GetChannelString() {
  return opera_version_info::ChannelToString(GetChannel());
}

// static
version_info::Channel GetChannel() {
  return opera_version_info::GetChannel();
}

}  // namespace chrome
