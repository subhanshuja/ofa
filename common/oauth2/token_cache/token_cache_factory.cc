// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/token_cache/token_cache_factory.h"

#include <memory>
#include <string>

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

#include "common/oauth2/diagnostics/diagnostic_service_factory.h"
#include "common/oauth2/token_cache/token_cache_impl.h"
#include "common/oauth2/token_cache/webdata_client_factory.h"
#include "common/sync/sync_config.h"

namespace opera {
namespace oauth2 {

// static
TokenCache* TokenCacheFactory::GetForProfile(Profile* profile) {
  return static_cast<TokenCache*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
TokenCache* TokenCacheFactory::GetForProfileIfExists(Profile* profile) {
  return static_cast<TokenCache*>(
      GetInstance()->GetServiceForBrowserContext(profile, false));
}

TokenCacheFactory* TokenCacheFactory::GetInstance() {
  return base::Singleton<TokenCacheFactory>::get();
}

TokenCacheFactory::TokenCacheFactory()
    : BrowserContextKeyedServiceFactory(
          "OperaOAuth2TokenCache",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(DiagnosticServiceFactory::GetInstance());
  DependsOn(WebdataClientFactory::GetInstance());
}

TokenCacheFactory::~TokenCacheFactory() {}

KeyedService* TokenCacheFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  if (!opera::SyncConfig::UsesOAuth2()) {
    return nullptr;
  }

  Profile* profile = Profile::FromBrowserContext(context);
  DCHECK(profile);

  DiagnosticService* diagnostic_service =
      DiagnosticServiceFactory::GetForProfile(profile);
  DCHECK(diagnostic_service);

  WebdataClient* webdata_client = WebdataClientFactory::GetForProfile(profile);
  DCHECK(webdata_client);

  TokenCacheImpl* token_cache(new TokenCacheImpl(diagnostic_service));
  token_cache->Initialize(webdata_client);
  return token_cache;
}
}  // namespace oauth2
}  // namespace opera
