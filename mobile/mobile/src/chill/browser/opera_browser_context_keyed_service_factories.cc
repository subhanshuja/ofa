// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#include "chill/browser/opera_browser_context_keyed_service_factories.h"

#include "chill/browser/login/oauth2_account_service_factory.h"
#include "common/bookmarks/duplicate_tracker_factory.h"
#include "common/chrome_imports/chrome/browser/invalidation/profile_invalidation_provider_factory.h"

namespace opera {

// Only root nodes of the dependency graph needs to be listed here, as properly
// defined service factories instantiate its dependencies as they register them
// with the dependency manager in their constructors.
void EnsureBrowserContextKeyedServiceFactoriesBuilt() {
  DuplicateTrackerFactory::GetInstance();
  OAuth2AccountServiceFactory::GetInstance();
  invalidation::ProfileInvalidationProviderFactory::GetInstance();
}

}  // namespace opera
