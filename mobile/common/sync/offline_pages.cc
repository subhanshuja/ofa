// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chrome/browser/android/offline_pages/offline_page_model_factory.h"
#include "components/offline_pages/offline_page_feature.h"

namespace offline_pages {

bool IsOfflinePagesEnabled() {
  return false;
}

// static
OfflinePageModelFactory* OfflinePageModelFactory::GetInstance() {
  return nullptr;
}

// static
OfflinePageModel* OfflinePageModelFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return nullptr;
}

}  // namespace offline_pages
