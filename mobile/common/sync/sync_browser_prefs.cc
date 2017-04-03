// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#include "mobile/common/sync/sync_browser_prefs.h"

#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/sync/driver/sync_prefs.h"
#include "common/sync/sync_login_data_store_impl.h"
#include "mobile/common/sync/sync_credentials_store.h"
#include "mobile/common/sync/sync_profile.h"

namespace mobile {

void RegisterUserProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  syncer::SyncPrefs::RegisterProfilePrefs(registry);
  opera::SyncLoginDataStoreImpl::RegisterProfilePrefs(registry);
  SyncCredentialsStore::RegisterProfilePrefs(registry);
  SyncProfile::RegisterProfilePrefs(registry);
  HostContentSettingsMap::RegisterProfilePrefs(registry);
}

}  // namespace mobile
