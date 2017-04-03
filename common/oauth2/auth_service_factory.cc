// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/auth_service_factory.h"

#include <memory>
#include <string>

#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "base/time/default_clock.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "chrome/browser/web_data_service_factory.h"

#include "common/oauth2/auth_service_impl.h"
#include "common/oauth2/device_name/device_name_service_factory.h"
#include "common/oauth2/diagnostics/diagnostic_service_factory.h"
#include "common/oauth2/migration/oauth1_migrator_impl.h"
#include "common/oauth2/network/network_request_manager_impl.h"
#include "common/oauth2/pref_names.h"
#include "common/oauth2/session/persistent_session_impl.h"
#include "common/oauth2/token_cache/token_cache_factory.h"
#include "common/oauth2/token_cache/token_cache_impl.h"
#include "common/oauth2/token_cache/webdata_client_factory.h"
#include "common/oauth2/token_cache/webdata_client.h"
#include "common/oauth2/util/init_params.h"
#include "common/oauth2/util/util.h"
#include "common/constants/pref_names.h"
#include "common/sync/sync_config.h"
#include "common/sync/sync_login_data_store_impl.h"

namespace opera {
namespace oauth2 {

// static
AuthService* AuthServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<AuthService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
AuthService* AuthServiceFactory::GetForProfileIfExists(Profile* profile) {
  return static_cast<AuthService*>(
      GetInstance()->GetServiceForBrowserContext(profile, false));
}

AuthServiceFactory* AuthServiceFactory::GetInstance() {
  return base::Singleton<AuthServiceFactory>::get();
}

AuthServiceFactory::AuthServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "OperaOAuth2Service",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(DiagnosticServiceFactory::GetInstance());
  DependsOn(DeviceNameServiceFactory::GetInstance());
  DependsOn(TokenCacheFactory::GetInstance());
}

AuthServiceFactory::~AuthServiceFactory() {}

void AuthServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  PersistentSessionImpl::RegisterProfilePrefs(registry);
}

KeyedService* AuthServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  if (!opera::SyncConfig::UsesOAuth2()) {
    return nullptr;
  }

  const GURL oauth1_base_url = SyncConfig::AuthServerURL();
  const GURL oauth2_base_url = SyncConfig::OAuth2ServerURL();
  const std::string oauth1_auth_service_id = SyncConfig::SyncAuthService();
  const std::string oauth1_client_id = SyncConfig::ClientKey();
  const std::string oauth1_client_secret = SyncConfig::ClientSecret();
  const std::string oauth2_client_id = SyncConfig::OAuth2ClientId();

  Profile* profile = Profile::FromBrowserContext(context);
  DCHECK(profile);
  AuthServiceImpl* service = new AuthServiceImpl();
  DCHECK(service);
  DiagnosticService* diagnostic_service =
      DiagnosticServiceFactory::GetForProfile(profile);
  DCHECK(diagnostic_service);
  DeviceNameService* device_name_service =
      DeviceNameServiceFactory::GetForProfile(profile);
  DCHECK(device_name_service);

  std::unique_ptr<InitParams> init_params(new InitParams);
  init_params->diagnostic_service = diagnostic_service;
  init_params->pref_service = profile->GetPrefs();
  init_params->client_id = oauth2_client_id;
  TokenCache* token_cache = TokenCacheFactory::GetForProfile(profile);
  DCHECK(token_cache);
  init_params->oauth2_token_cache = token_cache;
  init_params->oauth2_session = base::WrapUnique(
      new PersistentSessionImpl(diagnostic_service, profile->GetPrefs()));

  scoped_refptr<base::SingleThreadTaskRunner> task_runner =
      base::ThreadTaskRunnerHandle::Get();

  NetworkRequestManagerImpl::RequestUrlsConfig url_config;
  url_config[RMUT_OAUTH1] = std::make_pair(
      oauth1_base_url, opera::SyncConfig::AllowInsecureAuthConnection());
  url_config[RMUT_OAUTH2] = std::make_pair(
      oauth2_base_url, opera::SyncConfig::AllowInsecureAccountsConnection());

  init_params->oauth2_network_request_manager =
      base::WrapUnique(new NetworkRequestManagerImpl(
          diagnostic_service, url_config, profile->GetRequestContext(),
          NetworkRequestManagerImpl::GetDefaultBackoffPolicy(), nullptr,
          base::WrapUnique(new base::DefaultClock), task_runner));

  init_params->oauth1_migrator = base::WrapUnique(new OAuth1MigratorImpl(
      diagnostic_service, init_params->oauth2_network_request_manager.get(),
      device_name_service, base::WrapUnique(new SyncLoginDataStoreImpl(
                               profile->GetPrefs(), prefs::kSyncLoginData)),
      oauth1_base_url, oauth2_base_url, oauth1_auth_service_id,
      oauth1_client_id, oauth1_client_secret, oauth2_client_id));

  init_params->device_name_service = device_name_service;
  init_params->task_runner = task_runner;

  service->Initialize(std::move(init_params));
  return service;
}
}  // namespace oauth2
}  // namespace opera
