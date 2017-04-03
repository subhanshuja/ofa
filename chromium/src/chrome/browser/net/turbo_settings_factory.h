// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHROME_BROWSER_NET_TURBO_SETTINGS_FACTORY_H_
#define CHROME_BROWSER_NET_TURBO_SETTINGS_FACTORY_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/refcounted_browser_context_keyed_service_factory.h"

class Profile;
class TurboSettings;

class TurboSettingsFactory
    : public RefcountedBrowserContextKeyedServiceFactory {
 public:
  // Returns the |TurboSettings| associated with the |profile|.
  //
  // This should only be called on the UI thread.
  static scoped_refptr<TurboSettings> GetForProfile(Profile* profile);

  static TurboSettingsFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<TurboSettingsFactory>;

  TurboSettingsFactory();
  ~TurboSettingsFactory() override;

  // |BrowserContextKeyedBaseFactory| methods:
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  scoped_refptr<RefcountedKeyedService> BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(TurboSettingsFactory);
};

#endif  // CHROME_BROWSER_NET_TURBO_SETTINGS_FACTORY_H_
