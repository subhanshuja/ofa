// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_AUTH_KEEPER_FACTORY_H_
#define COMMON_SYNC_SYNC_AUTH_KEEPER_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "chrome/browser/profiles/profile.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace opera {
namespace sync {
class SyncAuthKeeper;

// Singleton that owns all BookmarkModels and associates them with Profiles.
class SyncAuthKeeperFactory : public BrowserContextKeyedServiceFactory {
 public:
  static SyncAuthKeeper* GetForProfile(Profile* profile);

  static SyncAuthKeeper* GetForProfileIfExists(Profile* profile);

  static SyncAuthKeeperFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<SyncAuthKeeperFactory>;

  SyncAuthKeeperFactory();
  ~SyncAuthKeeperFactory() override;

  // BrowserContextKeyedServiceFactory implementation.
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(SyncAuthKeeperFactory);
};
}  // namespace sync
}  // namespace opera
#endif  // COMMON_SYNC_SYNC_AUTH_KEEPER_FACTORY_H_
