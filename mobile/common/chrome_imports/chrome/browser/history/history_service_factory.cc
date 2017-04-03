// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "chrome/browser/history/history_service_factory.h"

#include "base/memory/ptr_util.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/history/chrome_history_client.h"
#include "chrome/common/pref_names.h"
#if defined(OS_ANDROID)
#include "components/history/content/browser/content_visit_delegate.h"
#include "components/history/content/browser/history_database_helper.h"
#include "mobile/common/sync/sync_profile.h"
#endif
#if defined(OS_IOS)
#include "components/history/core/browser/visit_delegate.h"
#include "components/history/ios/browser/history_database_helper.h"
#endif
#include "components/history/core/browser/history_database_params.h"
#include "components/history/core/browser/history_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_service.h"

// static
history::HistoryService* HistoryServiceFactory::GetForProfile(
    Profile* profile,
    ServiceAccessType sat) {
  // If saving history is disabled, only allow explicit access.
  if (sat != ServiceAccessType::EXPLICIT_ACCESS &&
      profile->GetPrefs()->GetBoolean(prefs::kSavingBrowserHistoryDisabled)) {
    return nullptr;
  }

  return static_cast<history::HistoryService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
history::HistoryService* HistoryServiceFactory::GetForProfileIfExists(
    Profile* profile,
    ServiceAccessType sat) {
  // If saving history is disabled, only allow explicit access.
  if (sat != ServiceAccessType::EXPLICIT_ACCESS &&
      profile->GetPrefs()->GetBoolean(prefs::kSavingBrowserHistoryDisabled)) {
    return nullptr;
  }

  return static_cast<history::HistoryService*>(
      GetInstance()->GetServiceForBrowserContext(profile, false));
}

// static
HistoryServiceFactory* HistoryServiceFactory::GetInstance() {
  return base::Singleton<HistoryServiceFactory>::get();
}

HistoryServiceFactory::HistoryServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "HistoryService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(BookmarkModelFactory::GetInstance());
}

HistoryServiceFactory::~HistoryServiceFactory() {
}

KeyedService* HistoryServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = static_cast<Profile*>(context);
  std::unique_ptr<history::HistoryService> history_service(
      new history::HistoryService(
          base::WrapUnique(new ChromeHistoryClient(
              BookmarkModelFactory::GetForBrowserContext(profile))),
#if defined(OS_ANDROID) && !defined(OPERA_MINI)
          // We pass public mode BrowserContext instead of sync BrowserContext
          // so that VisitedLinkEventListener::Observe() properly associates
          // with VisitedLinkSlave running in public mode rendering process.
          // (VisitedLinkEventListener is created by VisitedLinkMaster created
          // by ContentVisitDelegate)
          base::WrapUnique(new history::ContentVisitDelegate(
              static_cast<mobile::SyncProfile*>(profile)->GetBrowserContext()))
#else
          nullptr
#endif
          ));
  if (!history_service->Init(
          history::HistoryDatabaseParamsForPath(profile->GetPath()))) {
    return nullptr;
  }

  return history_service.release();
}

content::BrowserContext* HistoryServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return context;
}
