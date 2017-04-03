// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/bookmarks/duplicate_tracker_factory.h"

#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"

#include "common/bookmarks/duplicate_tracker.h"
#include "common/sync/sync_config.h"

#if defined(OPERA_DESKTOP)
#include "desktop/common/favorites/favorite_duplicate_tracker_delegate.h"
#else  // OPERA_DESKTOP
#include "mobile/common/favorites/favorite_duplicate_tracker_delegate.h"
#endif  // OPERA_DESKTOP

namespace opera {
// static
DuplicateTracker* DuplicateTrackerFactory::GetForProfile(Profile* profile) {
  return static_cast<DuplicateTracker*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
DuplicateTracker* DuplicateTrackerFactory::GetForProfileIfExists(
    Profile* profile) {
  return static_cast<DuplicateTracker*>(
      GetInstance()->GetServiceForBrowserContext(profile, false));
}

DuplicateTrackerFactory* DuplicateTrackerFactory::GetInstance() {
  return base::Singleton<DuplicateTrackerFactory>::get();
}

DuplicateTrackerFactory::DuplicateTrackerFactory()
    : BrowserContextKeyedServiceFactory(
          "DuplicateTracker",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(BookmarkModelFactory::GetInstance());
  DependsOn(ProfileSyncServiceFactory::GetInstance());
#if defined(OPERA_DESKTOP)
  DependsOn(opera::sync::SyncAuthKeeperFactory::GetInstance());
#endif  // OPERA_DESKTOP
}

DuplicateTrackerFactory::~DuplicateTrackerFactory() {}

void DuplicateTrackerFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  // Default value set to true to ensure first model scan.
  registry->RegisterBooleanPref(opera::kOperaDeduplicationBookmarkModelChanged,
                                true);
  registry->RegisterInt64Pref(opera::kOperaDeduplicationLastSuccessfulRun, 0);
}

KeyedService* DuplicateTrackerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  if (!opera::SyncConfig::ShouldDeduplicateBookmarks()) {
    return nullptr;
  }

  Profile* profile = Profile::FromBrowserContext(context);
  browser_sync::ProfileSyncService* service =
      ProfileSyncServiceFactory::GetForProfile(profile);
  bookmarks::BookmarkModel* model =
      BookmarkModelFactory::GetForBrowserContext(profile);
  PrefService* pref_service = profile->GetPrefs();
  DCHECK(service);
  DCHECK(model);
  DuplicateTracker* tracker =
      new opera::DuplicateTracker(service, model, pref_service);
  DCHECK(tracker);
  tracker->SetDelegate(
      base::WrapUnique(new opera::FavoriteDuplicateTrackerDelegate()));
  tracker->Initialize();
  tracker->Start();
  return tracker;
}
}  // namespace opera
