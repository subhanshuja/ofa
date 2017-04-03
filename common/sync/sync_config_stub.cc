// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sync_config.h"

namespace opera {

const char* SyncConfig::ClientKey() {
  return nullptr;
}

const char* SyncConfig::ClientSecret() {
  return nullptr;
}

const char* SyncConfig::SyncServerURLSwitch() {
  return nullptr;
}

bool SyncConfig::UsesOAuth2() {
  return false;
}

const char* SyncConfig::OAuth2ClientId() {
  return nullptr;
}

bool SyncConfig::ShouldSynchronizeHistory() {
  return false;
}

bool SyncConfig::ShouldSynchronizePasswords() {
  return false;
}

bool SyncConfig::ShouldSynchronizePrefs() {
  return false;
}

bool SyncConfig::ShouldSynchronizeTabs() {
  return false;
}

bool SyncConfig::ShouldBypassTypeConfiguration() {
  return false;
}

bool SyncConfig::ShouldDeduplicateBookmarks() {
  return false;
}

bool SyncConfig::ShouldRemoveDuplicates() {
  return false;
}

const char* SyncConfig::SyncAuthService() {
  return nullptr;
}

bool SyncConfig::ShouldUseSmartTokenHandling() {
  return false;
}

bool SyncConfig::ShouldUseAutoPasswordRecovery() {
  return false;
}

bool SyncConfig::ShouldUseAuthTokenRecovery() {
  return false;
}

bool SyncConfig::AllowInsecureAuthConnection() {
  return false;
}

}  // namespace opera
