// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/favicon/favicon_service_factory.h"

#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/favicon/core/favicon_client.h"
#include "components/favicon/core/favicon_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

#if !defined(OPERA_MINI)
namespace {

class DummyFaviconClient : public favicon::FaviconClient {
 public:
  bool IsNativeApplicationURL(const GURL& url) override {
    return false;
  }

  base::CancelableTaskTracker::TaskId GetFaviconForNativeApplicationURL(
      const GURL& url,
      const std::vector<int>& desired_sizes_in_pixel,
      const favicon_base::FaviconResultsCallback& callback,
      base::CancelableTaskTracker* tracker) override {
    return base::CancelableTaskTracker::kBadTaskId;
  }
};

}  // namespace
#endif  // !OPERA_MINI

// static
favicon::FaviconService* FaviconServiceFactory::GetForProfile(
    Profile* profile, ServiceAccessType sat)
{
  return static_cast<favicon::FaviconService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
FaviconServiceFactory* FaviconServiceFactory::GetInstance() {
  return base::Singleton<FaviconServiceFactory>::get();
}

FaviconServiceFactory::FaviconServiceFactory()
    : BrowserContextKeyedServiceFactory(
        "FaviconService",
        BrowserContextDependencyManager::GetInstance()) {
}

FaviconServiceFactory::~FaviconServiceFactory() {
}

KeyedService* FaviconServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
#if defined(OPERA_MINI)
  return new favicon::FaviconService();
#else
  return new favicon::FaviconService(
      base::WrapUnique(new DummyFaviconClient()),
      HistoryServiceFactory::GetForProfile(
          Profile::FromBrowserContext(profile),
          ServiceAccessType::EXPLICIT_ACCESS));
#endif
}

bool FaviconServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}
