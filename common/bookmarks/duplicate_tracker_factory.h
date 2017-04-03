// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_BOOKMARKS_DUPLICATE_TRACKER_FACTORY_H_
#define COMMON_BOOKMARKS_DUPLICATE_TRACKER_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "chrome/browser/profiles/profile.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace opera {
class DuplicateTracker;

// Singleton that owns all BookmarkModels and associates them with Profiles.
class DuplicateTrackerFactory : public BrowserContextKeyedServiceFactory {
 public:
  static DuplicateTracker* GetForProfile(Profile* profile);

  static DuplicateTracker* GetForProfileIfExists(Profile* profile);

  static DuplicateTrackerFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<DuplicateTrackerFactory>;

  DuplicateTrackerFactory();
  ~DuplicateTrackerFactory() override;

  // BrowserContextKeyedServiceFactory implementation.
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(DuplicateTrackerFactory);
};
}  // namespace opera
#endif  // COMMON_BOOKMARKS_DUPLICATE_TRACKER_FACTORY_H_
