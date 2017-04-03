// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/device_name/device_name_service_factory.h"

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"

#include "common/oauth2/device_name/device_name_service_impl.h"
#include "common/oauth2/device_name/get_session_name.h"
#include "common/sync/sync_config.h"

namespace opera {
namespace oauth2 {

// static
DeviceNameService* DeviceNameServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<DeviceNameService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
DeviceNameService* DeviceNameServiceFactory::GetForProfileIfExists(
    Profile* profile) {
  return static_cast<DeviceNameService*>(
      GetInstance()->GetServiceForBrowserContext(profile, false));
}

DeviceNameServiceFactory* DeviceNameServiceFactory::GetInstance() {
  return base::Singleton<DeviceNameServiceFactory>::get();
}

DeviceNameServiceFactory::DeviceNameServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "DeviceNameServiceFactory",
          BrowserContextDependencyManager::GetInstance()) {}

DeviceNameServiceFactory::~DeviceNameServiceFactory() {}

void DeviceNameServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  DeviceNameServiceImpl::RegisterProfilePrefs(registry);
}

KeyedService* DeviceNameServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  if (!opera::SyncConfig::UsesOAuth2()) {
    return nullptr;
  }

  Profile* profile = Profile::FromBrowserContext(context);
  DCHECK(profile);

  DeviceNameServiceImpl::GetDeviceNameCallback get_device_name_callback =
      base::Bind(session_name::GetSessionNameImpl);
  DeviceNameService* service =
      new DeviceNameServiceImpl(profile->GetPrefs(), get_device_name_callback);
  return service;
}
}  // namespace oauth2
}  // namespace opera
