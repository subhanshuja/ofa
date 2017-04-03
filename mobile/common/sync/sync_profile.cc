// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/sync/sync_profile.h"

#include "base/memory/ptr_util.h"
#include "chrome/common/pref_names.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_service_factory.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/storage_partition.h"
#include "mobile/common/sync/sync_browser_prefs.h"

namespace mobile {

SyncProfile::SyncProfile(
    content::BrowserContext* context,
    scoped_refptr<base::SequencedTaskRunner> io_task_runner)
    : context_(context),
      pref_registry_(new user_prefs::PrefRegistrySyncable()),
      io_task_runner_(io_task_runner) {
  PrefServiceFactory factory;
  factory.SetUserPrefsFile(
      context->GetPath().Append(FILE_PATH_LITERAL("prefs.json")),
      io_task_runner_.get());
  pref_service_ = factory.Create(pref_registry_.get());

  RegisterUserProfilePrefs(pref_registry_.get());

  BrowserContextDependencyManager::GetInstance()
      ->RegisterProfilePrefsForServices(this, pref_registry_.get());
}

// static
void SyncProfile::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(prefs::kSavingBrowserHistoryDisabled, false);
  registry->RegisterStringPref(prefs::kAcceptLanguages, "en-US,en",
                               user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
}

net::URLRequestContextGetter* SyncProfile::GetRequestContext() {
  return BrowserContext::GetDefaultStoragePartition(context_)
      ->GetURLRequestContext();
}

}  // namespace mobile

namespace chrome {

// Returns the original browser context even for Incognito contexts.
content::BrowserContext* GetBrowserContextRedirectedInIncognito(
    content::BrowserContext* context) {
  return context;
}

// Returns non-NULL even for Incognito contexts so that a separate
// instance of a service is created for the Incognito context.
content::BrowserContext* GetBrowserContextOwnInstanceInIncognito(
    content::BrowserContext* context) {
  return context;
}

}  // namespace chrome
