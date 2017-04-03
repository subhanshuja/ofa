// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/invalidation/impl/invalidation_service_util.h"

#include <vector>

#include "base/base64.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/rand_util.h"
#include "components/invalidation/impl/invalidation_switches.h"

#if defined(OPERA_SYNC)
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "net/base/port_util.h"

#include "common/sync/sync_config.h"
#endif  // OPERA_SYNC

namespace invalidation {

notifier::NotifierOptions ParseNotifierOptions(
    const base::CommandLine& command_line) {
  notifier::NotifierOptions notifier_options;

#if defined(OPERA_SYNC)
  // Ensure the fallback to 443 is present.
  notifier_options.fallback_port =
      opera::SyncConfig::FallbackInvalidationPort();

  notifier_options.allow_insecure_connection =
    command_line.HasSwitch(switches::kSyncAllowInsecureXmppConnection);

  // We use the default if nothing found on the command line.
  notifier_options.xmpp_host_port = net::HostPortPair::FromString(
      opera::SyncConfig::DefaultInvalidationHostPort());

  if (command_line.HasSwitch(switches::kSyncNotificationHostPort)) {
    const std::string cl_value =
        command_line.GetSwitchValueASCII(switches::kSyncNotificationHostPort);

    std::vector<std::string> key_port = base::SplitString(
        cl_value, ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

    if (key_port.size() == 1) {
      // No port in command line, use the default set above.
      notifier_options.xmpp_host_port.set_host(key_port[0]);
    } else if (key_port.size() == 2) {
      // Only use the host/port found on the command line in case the data is
      // valid.
      int new_port;
      if (base::StringToInt(key_port[1], &new_port)) {
        if (net::IsPortValid(new_port)) {
          notifier_options.xmpp_host_port.set_host(key_port[0]);
          notifier_options.xmpp_host_port.set_port(new_port);
        }
      }
    }
  }

  if (command_line.HasSwitch(switches::kSyncNotificationFallbackPort)) {
    const std::string cl_value = command_line.
        GetSwitchValueASCII(switches::kSyncNotificationFallbackPort);
    int cl_port = 0;
    if (base::StringToInt(cl_value, &cl_port) && cl_port > 0 &&
        net::IsPortValid(cl_port)) {
      notifier_options.fallback_port = cl_port;
    }
  }

  VLOG(1) << "XMPP address set to: " <<
      notifier_options.xmpp_host_port.ToString();
  VLOG(1) << "XMPP fallback port is set to: " <<
      notifier_options.fallback_port;
  VLOG(1) << "XMPP insecure connection allowed: " <<
      notifier_options.allow_insecure_connection;
#else
  if (command_line.HasSwitch(switches::kSyncNotificationHostPort)) {
    notifier_options.xmpp_host_port =
        net::HostPortPair::FromString(
            command_line.GetSwitchValueASCII(
                switches::kSyncNotificationHostPort));
    DVLOG(1) << "Using " << notifier_options.xmpp_host_port.ToString()
             << " for test sync notification server.";
  }

  notifier_options.allow_insecure_connection =
      command_line.HasSwitch(switches::kSyncAllowInsecureXmppConnection);
  DVLOG_IF(1, notifier_options.allow_insecure_connection)
      << "Allowing insecure XMPP connections.";
#endif  // OPERA_SYNC

  return notifier_options;
}

std::string GenerateInvalidatorClientId() {
  // Generate a GUID with 128 bits worth of base64-encoded randomness.
  // This format is similar to that of sync's cache_guid.
  const int kGuidBytes = 128 / 8;
  std::string guid;
  base::Base64Encode(base::RandBytesAsString(kGuidBytes), &guid);
  DCHECK(!guid.empty());
  return guid;
}

}  // namespace invalidation
