// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_DEVICE_NAME_DEVICE_NAME_SERVICE_FACTORY_H_
#define COMMON_OAUTH2_DEVICE_NAME_DEVICE_NAME_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "chrome/browser/profiles/profile.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace opera {
namespace oauth2 {

class DeviceNameService;

class DeviceNameServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static DeviceNameService* GetForProfile(Profile* profile);
  static DeviceNameService* GetForProfileIfExists(Profile* profile);
  static DeviceNameServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<DeviceNameServiceFactory>;

  DeviceNameServiceFactory();
  ~DeviceNameServiceFactory() override;

  // BrowserContextKeyedServiceFactory implementation.
  void RegisterProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry) override;

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(DeviceNameServiceFactory);
};
}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_DEVICE_NAME_DEVICE_NAME_SERVICE_FACTORY_H_
