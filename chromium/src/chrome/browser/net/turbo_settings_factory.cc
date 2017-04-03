// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chrome/browser/net/turbo_settings_factory.h"

#include "chrome/browser/net/turbo_settings.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/core/refcounted_keyed_service.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "content/public/browser/browser_thread.h"

// static
scoped_refptr<TurboSettings> TurboSettingsFactory::GetForProfile(
    Profile* profile) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return static_cast<TurboSettings*>(
      GetInstance()->GetServiceForBrowserContext(profile, true).get());
}

// static
TurboSettingsFactory* TurboSettingsFactory::GetInstance() {
  return base::Singleton<TurboSettingsFactory>::get();
}

TurboSettingsFactory::TurboSettingsFactory()
    : RefcountedBrowserContextKeyedServiceFactory(
          "TurboSettings",
          BrowserContextDependencyManager::GetInstance()) {}

TurboSettingsFactory::~TurboSettingsFactory() {}

void TurboSettingsFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  TurboSettings::RegisterProfilePrefs(registry);
}

content::BrowserContext* TurboSettingsFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // The incognito profile should have its own setting.
  // We will not use them anyway because turbo != incognito.
  return chrome::GetBrowserContextOwnInstanceInIncognito(context);
}

scoped_refptr<RefcountedKeyedService>
TurboSettingsFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new TurboSettings(Profile::FromBrowserContext(context)->GetPrefs());
}
