// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2017 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#include "chill/browser/login/oauth2_account_service_factory.h"

#include "base/memory/singleton.h"
#include "common/oauth2/auth_service_factory.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

#include "chill/browser/login/oauth2_account_service.h"

namespace opera {

// static
OAuth2AccountService* OAuth2AccountServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<OAuth2AccountService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

OAuth2AccountServiceFactory* OAuth2AccountServiceFactory::GetInstance() {
  return base::Singleton<OAuth2AccountServiceFactory>::get();
}

OAuth2AccountServiceFactory::OAuth2AccountServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "OAuth2Service",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(oauth2::AuthServiceFactory::GetInstance());
}

OAuth2AccountServiceFactory::~OAuth2AccountServiceFactory() {}

KeyedService* OAuth2AccountServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  oauth2::AuthService* auth_service =
      oauth2::AuthServiceFactory::GetForProfile(profile);

  return new OAuth2AccountService(auth_service, profile->GetRequestContext());
}

}  // namespace opera
