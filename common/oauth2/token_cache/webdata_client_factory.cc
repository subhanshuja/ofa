// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/token_cache/webdata_client_factory.h"

#include <memory>
#include <string>

#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "chrome/browser/web_data_service_factory.h"

#include "common/oauth2/token_cache/token_cache_webdata.h"
#include "common/oauth2/token_cache/webdata_client_impl.h"
#include "common/oauth2/token_cache/testing_webdata_client.h"
#include "common/sync/sync_config.h"

namespace opera {
namespace oauth2 {

// static
WebdataClient* WebdataClientFactory::GetForProfile(Profile* profile) {
  return static_cast<WebdataClient*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
WebdataClient* WebdataClientFactory::GetForProfileIfExists(Profile* profile) {
  return static_cast<WebdataClient*>(
      GetInstance()->GetServiceForBrowserContext(profile, false));
}

WebdataClientFactory* WebdataClientFactory::GetInstance() {
  return base::Singleton<WebdataClientFactory>::get();
}

WebdataClientFactory::WebdataClientFactory()
    : BrowserContextKeyedServiceFactory(
          "OperaWebdataClient",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(WebDataServiceFactory::GetInstance());
}

WebdataClientFactory::~WebdataClientFactory() {}

KeyedService* WebdataClientFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  if (!opera::SyncConfig::UsesOAuth2()) {
    return nullptr;
  }

  Profile* profile = Profile::FromBrowserContext(context);
  DCHECK(profile);

  scoped_refptr<TokenCacheWebData> webdata =
      WebDataServiceFactory::GetTokenCacheWebDataForProfile(
          profile, ServiceAccessType::IMPLICIT_ACCESS);
  DCHECK(webdata);
  WebdataClientImpl* webdata_client = new WebdataClientImpl;
  webdata_client->Initialize(webdata);

  return webdata_client;
}

// static
std::unique_ptr<KeyedService>
WebdataClientFactory::BuildTestingServiceInstanceFor(
    content::BrowserContext* context) {
  return base::WrapUnique(new TestingWebdataClient);
}

}  // namespace oauth2
}  // namespace opera
