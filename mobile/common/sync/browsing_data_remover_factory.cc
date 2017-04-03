// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/browsing_data_remover_factory.h"

#include "base/memory/singleton.h"
#include "chrome/browser/browsing_data/browsing_data_remover.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/password_manager/password_store_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

// static
BrowsingDataRemoverFactory* BrowsingDataRemoverFactory::GetInstance() {
  return base::Singleton<BrowsingDataRemoverFactory>::get();
}

// static
BrowsingDataRemover* BrowsingDataRemoverFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<BrowsingDataRemover*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

BrowsingDataRemoverFactory::BrowsingDataRemoverFactory()
    : BrowserContextKeyedServiceFactory(
          "BrowsingDataRemover",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(HistoryServiceFactory::GetInstance());
  DependsOn(PasswordStoreFactory::GetInstance());
}

BrowsingDataRemoverFactory::~BrowsingDataRemoverFactory() {}

content::BrowserContext* BrowsingDataRemoverFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return static_cast<Profile*>(context);
}

KeyedService* BrowsingDataRemoverFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new BrowsingDataRemover(context);
}
