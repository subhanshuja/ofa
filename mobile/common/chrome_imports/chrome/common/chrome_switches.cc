// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chrome/common/chrome_switches.h"

namespace switches {

const char kDisableSyncTypes[]                  = "disable-sync-types";

const char kEnableSyncSyncedNotifications[]     =
    "enable-sync-synced-notifications";

const char kSyncShortInitialRetryOverride[]     =
    "sync-short-initial-retry-override";

const char kSyncServiceURL[]                    = "sync-url";

const char kSyncDisableDeferredStartup[]        =
    "sync-disable-deferred-startup";
const char kSyncDeferredStartupTimeoutSeconds[] =
    "sync-deferred-startup-timeout-seconds";
const char kSyncEnableGetUpdateAvoidance[]      =
    "sync-enable-get-update-avoidance";

const char kSyncEnableRollback[]                = "enable-sync-rollback";

const char kSyncDisableRollback[]               = "disable-sync-rollback";

// Enables clearing of sync data when a user enables passphrase encryption.
const char kSyncEnableClearDataOnPassphraseEncryption[] =
    "enable-clear-sync-data-on-passphrase-encryption";

const char kEnableBookmarkUndo[]            = "enable-bookmark-undo";

}  // namespace switches
