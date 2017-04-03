// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/web_data_service_factory.h"

#include "base/memory/singleton.h"
#include "chrome/browser/profiles/profile.h"
#include "common/oauth2/token_cache/token_cache_webdata.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/webdata_services/web_data_service_wrapper.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace {
// Callback to show error dialog on profile load error.
void ProfileErrorCallback(WebDataServiceWrapper::ErrorType error_type,
                          sql::InitStatus status,
                          const std::string& diagnostics) {}
}

WebDataServiceFactory::WebDataServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "WebDataService",
          BrowserContextDependencyManager::GetInstance()) {
  // WebDataServiceFactory has no dependecies.
}

WebDataServiceFactory::~WebDataServiceFactory() {}

// static
WebDataServiceWrapper* WebDataServiceFactory::GetForProfile(
    Profile* profile,
    ServiceAccessType access_type) {
  // If |access_type| starts being used for anything other than this
  // DCHECK, we need to start taking it as a parameter to
  // the *WebDataService::FromBrowserContext() functions (see above).
  DCHECK(access_type != ServiceAccessType::IMPLICIT_ACCESS ||
         !profile->IsOffTheRecord());
  return static_cast<WebDataServiceWrapper*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
scoped_refptr<autofill::AutofillWebDataService>
WebDataServiceFactory::GetAutofillWebDataForProfile(
    Profile* profile,
    ServiceAccessType access_type) {
  return nullptr;
}

// static
scoped_refptr<opera::oauth2::TokenCacheWebData>
WebDataServiceFactory::GetTokenCacheWebDataForProfile(
    Profile* profile,
    ServiceAccessType access_type) {
  WebDataServiceWrapper* wrapper =
      WebDataServiceFactory::GetForProfile(profile, access_type);
  // |wrapper| can be null in Incognito mode.
  return wrapper ? wrapper->GetOperaTokenCacheWebData()
                 : scoped_refptr<opera::oauth2::TokenCacheWebData>(nullptr);
}

// static
WebDataServiceFactory* WebDataServiceFactory::GetInstance() {
  return base::Singleton<WebDataServiceFactory>::get();
}

content::BrowserContext* WebDataServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return context;
}

KeyedService* WebDataServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  const base::FilePath& profile_path = context->GetPath();
  return new WebDataServiceWrapper(
      profile_path,
      content::BrowserThread::GetTaskRunnerForThread(BrowserThread::UI),
      content::BrowserThread::GetTaskRunnerForThread(BrowserThread::DB),
      &ProfileErrorCallback);
}

bool WebDataServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}
