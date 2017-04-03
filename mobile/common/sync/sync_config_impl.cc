// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sync_config.h"

#include "base/command_line.h"
#include "components/sync/driver/sync_driver_switches.h"
#include "mobile/common/sync/sync_switches.h"

namespace opera {

// static
const char* SyncConfig::ClientKey() {
  return SYNC_AUTH_CLIENT_KEY;
}

// static
const char* SyncConfig::ClientSecret() {
  return SYNC_AUTH_CLIENT_SECRET;
}

// static
const char* SyncConfig::SyncServerURLSwitch() {
  return ::switches::kSyncServiceURL;
}

// static
bool SyncConfig::UsesOAuth2() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      opera::switches::kEnableOAuth2);
}

// static
const char* SyncConfig::OAuth2ClientId() {
  return "ofa";
}

// static
bool SyncConfig::ShouldSynchronizeHistory() {
  return true;
}

// static
bool SyncConfig::ShouldSynchronizePrefs() {
  return false;
}

// static
bool SyncConfig::ShouldSynchronizeTabs() {
  return true;
}

// static
const char* SyncConfig::SyncAuthService() {
  return SYNC_AUTH_SERVICE;
}

// static
bool SyncConfig::ShouldUseSmartTokenHandling() {
  return false;
}

// static
bool SyncConfig::ShouldUseAutoPasswordRecovery() {
  return false;
}

// static
bool SyncConfig::ShouldSynchronizePasswords() {
  return false;
}

// static
bool SyncConfig::ShouldBypassTypeConfiguration() {
  DCHECK(ShouldSynchronizePasswords());
  return false;
}

// static
bool SyncConfig::ShouldDeduplicateBookmarks() {
  return true;
}

// static
bool SyncConfig::ShouldRemoveDuplicates() {
  return true;
}

// static
bool SyncConfig::ShouldUseAuthTokenRecovery() {
  return false;
}

// static
bool SyncConfig::AllowInsecureAuthConnection() {
  return false;
}

// static
bool SyncConfig::AllowInsecureAccountsConnection() {
  return false;
}

}  // namespace opera
