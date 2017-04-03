// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sync_auth_keeper_factory.h"

#include <string>

#include "base/memory/singleton.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"

#include "common/sync/pref_names.h"
#include "common/sync/sync_auth_keeper.h"
#include "common/sync/sync_config.h"

namespace opera {
namespace sync {
// static
SyncAuthKeeper* SyncAuthKeeperFactory::GetForProfile(Profile* profile) {
  return static_cast<SyncAuthKeeper*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
SyncAuthKeeper* SyncAuthKeeperFactory::GetForProfileIfExists(Profile* profile) {
  return static_cast<SyncAuthKeeper*>(
      GetInstance()->GetServiceForBrowserContext(profile, false));
}

SyncAuthKeeperFactory* SyncAuthKeeperFactory::GetInstance() {
  return base::Singleton<SyncAuthKeeperFactory>::get();
}

SyncAuthKeeperFactory::SyncAuthKeeperFactory()
    : BrowserContextKeyedServiceFactory(
          "SyncAuthKeeper",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ProfileSyncServiceFactory::GetInstance());
}

SyncAuthKeeperFactory::~SyncAuthKeeperFactory() {}

void SyncAuthKeeperFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterIntegerPref(kOperaSyncAuthTokenLostReasonPref, 0);
  registry->RegisterBooleanPref(kOperaSyncAuthTokenLostPref, 0);
  registry->RegisterInt64Pref(kOperaSyncLastSessionDurationPref,
                              0);
  registry->RegisterStringPref(kOperaSyncLastSessionIdPref,
                               std::string());
  registry->RegisterIntegerPref(kOperaSyncLastSessionLogoutReasonPref, 0);
}

KeyedService* SyncAuthKeeperFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  if (!SyncConfig::ShouldUseAuthTokenRecovery()) {
    return nullptr;
  }

  if (SyncConfig::UsesOAuth2()) {
    return nullptr;
  }

  Profile* profile = Profile::FromBrowserContext(context);
  DCHECK(profile);
  PrefService* pref_service = profile->GetPrefs();
  DCHECK(pref_service);
  browser_sync::ProfileSyncService* sync_service =
      ProfileSyncServiceFactory::GetForProfile(profile);
  DCHECK(sync_service);
  SyncAuthKeeper* keeper = new SyncAuthKeeper(sync_service, pref_service);
  DCHECK(keeper);
  keeper->Initialize();
  return keeper;
}
}  // namespace sync
}  // namespace opera
