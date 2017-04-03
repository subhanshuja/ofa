// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sync_config.h"

#include <string>

#include "base/command_line.h"
#include "base/logging.h"
#include "chrome/common/chrome_switches.h"

#include "common/oauth2/util/scopes.h"

namespace opera {

namespace {

const char kSyncServerURL[] = "https://sync.opera.com/api/sync";
const char kInvalidationServerURL[] = "push.opera.com:5222";
const int kFallbackInvalidationPort = 443;
const char kAuthServerURL[] = "https://auth.opera.com/";
const char kSyncPanelURL[] = "https://sync.opera.com/web/";
// TODO(mzajaczkowski): Update for production server.
const char kDefaultOAuth2BaseURL[] =
    "https://accounts.opera.com/";

// If |command_line_switch| is present in |command_line| and contains a
// non-empty, valid URL, it is returned. Otherwise, |default_url| is returned.
GURL GetUrlFromCommandLine(const base::CommandLine& command_line,
                           const char* command_line_switch,
                           const char* default_url) {
  const std::string value(
      command_line.GetSwitchValueASCII(command_line_switch));
  if (!value.empty()) {
    const GURL custom_url(value);
    if (custom_url.is_valid()) {
      return custom_url;
    } else {
      LOG(WARNING)
          << "The following URL specified at the command-line with switch --"
          << std::string(command_line_switch) << " is invalid: " << value
          << " using default instead: " << std::string(default_url);
    }
  }
  GURL result(default_url);
  DCHECK(result.is_valid());
  return result;
}

}  // namespace

const char kSyncAuthURLSwitch[] = "sync-auth-url";
const char kSyncAccountsURLSwitch[] = "sync-accounts-url";

// static
GURL SyncConfig::AuthServerURL() {
  return AuthServerURL(*base::CommandLine::ForCurrentProcess());
}

// static
GURL SyncConfig::AuthServerURL(const base::CommandLine& command_line) {
  return GetUrlFromCommandLine(command_line, kSyncAuthURLSwitch, kAuthServerURL)
      .GetOrigin();
}

// static
GURL SyncConfig::SyncServerURL() {
  return SyncServerURL(*base::CommandLine::ForCurrentProcess());
}

// static
GURL SyncConfig::SyncServerURL(const base::CommandLine& command_line) {
  return GetUrlFromCommandLine(command_line, SyncServerURLSwitch(),
                               kSyncServerURL);
}

// static
GURL SyncConfig::SyncPanelURL() {
  return GURL(kSyncPanelURL);
}

// static
const char* SyncConfig::DefaultInvalidationHostPort() {
  return kInvalidationServerURL;
}

// static
int SyncConfig::FallbackInvalidationPort() {
  return kFallbackInvalidationPort;
}

// static
GURL SyncConfig::OAuth2ServerURL() {
  return GetUrlFromCommandLine(*base::CommandLine::ForCurrentProcess(),
                               kSyncAccountsURLSwitch, kDefaultOAuth2BaseURL);
}

// static
const char* SyncConfig::OAuth2ScopeSync() {
  return oauth2::scope::kSync;
}

// static
const char* SyncConfig::OAuth2ScopeXmpp() {
  return oauth2::scope::kPushNotifications;
}

// static
const char* SyncConfig::OAuth2ScopeThumbnailSync() {
  return oauth2::scope::kContentBroker;
}

// static
syncer::ModelTypeSet SyncConfig::OperaUserTypes() {
  syncer::ModelTypeSet set;
  set.Put(syncer::BOOKMARKS);
  if (ShouldSynchronizePrefs())
    set.Put(syncer::PREFERENCES);
  if (ShouldSynchronizePasswords())
    set.Put(syncer::PASSWORDS);
  if (ShouldSynchronizeHistory()) {
    set.Put(syncer::TYPED_URLS);
    set.Put(syncer::HISTORY_DELETE_DIRECTIVES);
  }
  if (ShouldSynchronizeTabs()) {
    set.Put(syncer::SESSIONS);
    set.Put(syncer::PROXY_TABS);
  }
  return set;
}

// static
syncer::ModelTypeSet SyncConfig::OperaSensitiveTypes() {
  syncer::ModelTypeSet set;
  set.Put(syncer::PASSWORDS);
  return set;
}

}  // namespace opera
