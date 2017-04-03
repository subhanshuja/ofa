// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/password_manager/password_store_factory.h"

#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/password_manager/core/browser/password_store.h"

// static
scoped_refptr<password_manager::PasswordStore> PasswordStoreFactory::GetForProfile(
    Profile* profile, ServiceAccessType sat) {
  return NULL;
}

// static
PasswordStoreFactory* PasswordStoreFactory::GetInstance() {
  return base::Singleton<PasswordStoreFactory>::get();
}

// static
void PasswordStoreFactory::OnPasswordsSyncedStatePotentiallyChanged(
    Profile* profile) {
}

PasswordStoreFactory::PasswordStoreFactory()
    : BrowserContextKeyedServiceFactory(
        "PasswordStore",
        BrowserContextDependencyManager::GetInstance()) {
}

PasswordStoreFactory::~PasswordStoreFactory() {}

KeyedService* PasswordStoreFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return NULL;
}
