// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_TOKEN_CACHE_WEBDATA_CLIENT_FACTORY_H_
#define COMMON_OAUTH2_TOKEN_CACHE_WEBDATA_CLIENT_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}  // namespace base

namespace opera {
namespace oauth2 {

class WebdataClient;

class WebdataClientFactory : public BrowserContextKeyedServiceFactory {
 public:
  static WebdataClient* GetForProfile(Profile* profile);
  static WebdataClient* GetForProfileIfExists(Profile* profile);
  static WebdataClientFactory* GetInstance();

  static std::unique_ptr<KeyedService> BuildTestingServiceInstanceFor(
      content::BrowserContext* context);

 private:
  friend struct base::DefaultSingletonTraits<WebdataClientFactory>;

  WebdataClientFactory();
  ~WebdataClientFactory() override;

  // BrowserContextKeyedServiceFactory implementation.
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(WebdataClientFactory);
};
}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_TOKEN_CACHE_WEBDATA_CLIENT_FACTORY_H_
