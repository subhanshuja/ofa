// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/diagnostics/diagnostic_service_factory.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

#include "common/oauth2/diagnostics/diagnostic_service.h"
#include "common/sync/sync_config.h"

namespace opera {
namespace oauth2 {
// static
DiagnosticService* DiagnosticServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<DiagnosticService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
DiagnosticService* DiagnosticServiceFactory::GetForProfileIfExists(
    Profile* profile) {
  return static_cast<DiagnosticService*>(
      GetInstance()->GetServiceForBrowserContext(profile, false));
}

DiagnosticServiceFactory* DiagnosticServiceFactory::GetInstance() {
  return base::Singleton<DiagnosticServiceFactory>::get();
}

DiagnosticServiceFactory::DiagnosticServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "OperaOAuth2DiagnosticService",
          BrowserContextDependencyManager::GetInstance()) {}

DiagnosticServiceFactory::~DiagnosticServiceFactory() {}

KeyedService* DiagnosticServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  if (!opera::SyncConfig::UsesOAuth2()) {
    return nullptr;
  }

  DiagnosticService* service = new DiagnosticService(100u);

  return service;
}
}  // namespace oauth2
}  // namespace opera
