// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/sync_util.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "build/build_config.h"
#include "components/sync/driver/sync_driver_switches.h"
#include "url/gurl.h"

#if defined(OPERA_SYNC)
#include "common/sync/sync_config.h"
#include "mobile/common/sync/opera_version_info.h"
#else
#include "chrome/common/channel_info.h"
#include "components/version_info/version_info.h"
#endif  // OPERA_SYNC

namespace {

// Returns string that represents system in UserAgent.
std::string GetSystemString(bool is_tablet) {
  std::string system;
#if defined(OS_CHROMEOS)
  system = "CROS ";
#elif defined(OS_ANDROID)
  if (is_tablet) {
    system = "ANDROID-TABLET ";
  } else {
    system = "ANDROID-PHONE ";
  }
#elif defined(OS_IOS)
  if (is_tablet) {
    system = "IOS-TABLET ";
  } else {
    system = "IOS-PHONE ";
  }
#elif defined(OS_WIN)
  system = "WIN ";
#elif defined(OS_LINUX)
  system = "LINUX ";
#elif defined(OS_FREEBSD)
  system = "FREEBSD ";
#elif defined(OS_OPENBSD)
  system = "OPENBSD ";
#elif defined(OS_MACOSX)
  system = "MAC ";
#endif
  return system;
}

}  // namespace

namespace syncer {
namespace internal {

const char* kSyncServerUrl = "https://clients4.google.com/chrome-sync";

const char* kSyncDevServerUrl = "https://clients4.google.com/chrome-sync/dev";

#if defined(OPERA_SYNC)
std::string FormatUserAgentForSync(const std::string& system) {
#else
std::string FormatUserAgentForSync(const std::string& system,
                                   version_info::Channel channel) {
#endif  // OPERA_SYNC
  std::string user_agent;
#if defined(OPERA_SYNC)
  user_agent = "Opera ";
#else
  user_agent = "Chrome ";
#endif  // OPERA_SYNC
  user_agent += system;
#if defined(OPERA_SYNC)
  user_agent += opera_version_info::GetVersionNumber();
#else
  user_agent += version_info::GetVersionNumber();
#endif  // OPERA_DESKTOP

#if defined(OPERA_SYNC)
  user_agent += " (" + opera_version_info::GetProductName() + " " +
                opera_version_info::GetProductNameAndVersionForUserAgent() +
                ")";
#elif defined(OPERA_SYNC)
  user_agent += " (" + version_info::GetProductName() + " " +
                version_info::GetProductNameAndVersionForUserAgent() + ")";
#else
  user_agent += " (" + version_info::GetLastChange() + ")";
#endif  // OPERA_SYNC && OPERA_DESKTOP

#if defined(OPERA_SYNC)
  user_agent +=
      " channel(" +
      opera_version_info::ChannelToString(opera_version_info::GetChannel()) +
      ")";
#else
  if (!version_info::IsOfficialBuild()) {
    user_agent += "-devel";
  } else {
#if defined(OPERA_SYNC)
    user_agent += " channel(" +
        version_info::GetChannelString(chrome::GetChannel()) + ")";
#else
    user_agent += " channel(" + version_info::GetChannelString(channel) + ")";
#endif  // OPERA_SYNC
  }
#endif  // OPERA_SYNC && OPERA_DESKTOP
  return user_agent;
}

}  // namespace internal

GURL GetSyncServiceURL(const base::CommandLine& command_line,
                       version_info::Channel channel) {
#if defined(OPERA_SYNC)
  return opera::SyncConfig::SyncServerURL(command_line);
#else
  // By default, dev, canary, and unbranded Chromium users will go to the
  // development servers. Development servers have more features than standard
  // sync servers. Users with officially-branded Chrome stable and beta builds
  // will go to the standard sync servers.
  GURL result(internal::kSyncDevServerUrl);

  if (channel == version_info::Channel::STABLE ||
      channel == version_info::Channel::BETA) {
    result = GURL(internal::kSyncServerUrl);
  }

  // Override the sync server URL from the command-line, if sync server
  // command-line argument exists.
  if (command_line.HasSwitch(switches::kSyncServiceURL)) {
    std::string value(
        command_line.GetSwitchValueASCII(switches::kSyncServiceURL));
    if (!value.empty()) {
      GURL custom_sync_url(value);
      if (custom_sync_url.is_valid()) {
        result = custom_sync_url;
      } else {
        LOG(WARNING) << "The following sync URL specified at the command-line "
                     << "is invalid: " << value;
      }
    }
  }
  return result;
#endif  // OPERA_SYNC
}

#if defined(OPERA_SYNC)
std::string MakeUserAgentForSync(bool is_tablet) {
#else
std::string MakeUserAgentForSync(version_info::Channel channel,
                                 bool is_tablet) {
#endif  // OPERA_SYNC
  std::string system = GetSystemString(is_tablet);
#if defined(OPERA_SYNC)
  return internal::FormatUserAgentForSync(system);
#else
  return internal::FormatUserAgentForSync(system, channel);
#endif  // OPERA_SYNC
}

}  // namespace syncer
