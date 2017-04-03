// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_CONFIG_H_
#define COMMON_SYNC_SYNC_CONFIG_H_

#include "url/gurl.h"
#include "components/sync/base/model_type.h"

namespace base {
class CommandLine;
}  // namespace base

namespace opera {

// CommandLine switch that sets URL to Auth.
extern const char kSyncAuthURLSwitch[];
extern const char kSyncAccountsURLSwitch[];

struct SyncConfig {
  static const char* ClientKey();
  static const char* ClientSecret();
  static GURL AuthServerURL();
  static GURL AuthServerURL(const base::CommandLine& command_line);
  static GURL SyncServerURL();
  static GURL SyncServerURL(const base::CommandLine& command_line);
  static const char* SyncServerURLSwitch();
  static GURL SyncPanelURL();
  static const char* DefaultInvalidationHostPort();
  static int FallbackInvalidationPort();

  static bool UsesOAuth2();
  static GURL OAuth2ServerURL();
  static const char* OAuth2ClientId();
  static const char* OAuth2ScopeSync();
  static const char* OAuth2ScopeXmpp();
  static const char* OAuth2ScopeThumbnailSync();

  static bool ShouldSynchronizeHistory();
  static bool ShouldSynchronizePasswords();
  static bool ShouldSynchronizePrefs();
  static bool ShouldSynchronizeTabs();
  // Causes bypassing the new sync startup procedure (i.e. waiting untill user
  // chooses types). Has no effect in case the password sync feature is off.
  static bool ShouldBypassTypeConfiguration();
  static bool ShouldDeduplicateBookmarks();
  static bool ShouldRemoveDuplicates();
  static const char* SyncAuthService();
  static bool ShouldUseSmartTokenHandling();
  static bool ShouldUseAutoPasswordRecovery();
  static bool ShouldUseAuthTokenRecovery();

  static bool AllowInsecureAuthConnection();
  static bool AllowInsecureAccountsConnection();
  static syncer::ModelTypeSet OperaUserTypes();
  static syncer::ModelTypeSet OperaSensitiveTypes();
};

}  // namespace opera

#endif  // COMMON_SYNC_SYNC_CONFIG_H_
