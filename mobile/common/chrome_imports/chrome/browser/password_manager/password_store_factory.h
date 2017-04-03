// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PASSWORD_MANAGER_PASSWORD_STORE_FACTORY_H_
#define CHROME_BROWSER_PASSWORD_MANAGER_PASSWORD_STORE_FACTORY_H_

#include "base/memory/singleton.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/service_access_type.h"

namespace password_manager {
class PasswordStore;
}

class PasswordStoreFactory : public BrowserContextKeyedServiceFactory {
 public:
  static scoped_refptr<password_manager::PasswordStore> GetForProfile(
      Profile* profile, ServiceAccessType set);

  static PasswordStoreFactory* GetInstance();

  static void OnPasswordsSyncedStatePotentiallyChanged(Profile* profile);

 private:
  friend struct base::DefaultSingletonTraits<PasswordStoreFactory>;

  PasswordStoreFactory();
  virtual ~PasswordStoreFactory();

  virtual KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(PasswordStoreFactory);
};

#endif  // CHROME_BROWSER_PASSWORD_MANAGER_PASSWORD_STORE_FACTORY_H_
