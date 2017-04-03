// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2017 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#ifndef CHILL_BROWSER_LOGIN_OAUTH2_ACCOUNT_SERVICE_FACTORY_H_
#define CHILL_BROWSER_LOGIN_OAUTH2_ACCOUNT_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace base {
template <typename T>
struct DefaultSingletonTraits;
}

namespace opera {
class OAuth2AccountService;

class OAuth2AccountServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static OAuth2AccountService* GetForProfile(Profile* profile);
  static OAuth2AccountServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<OAuth2AccountServiceFactory>;

  OAuth2AccountServiceFactory();
  virtual ~OAuth2AccountServiceFactory();

  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(OAuth2AccountServiceFactory);
};

}  // namespace opera

#endif  // CHILL_BROWSER_LOGIN_OAUTH2_ACCOUNT_SERVICE_FACTORY_H_
