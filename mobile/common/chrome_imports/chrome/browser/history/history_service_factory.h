// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_HISTORY_HISTORY_SERVICE_FACTORY_H_
#define CHROME_BROWSER_HISTORY_HISTORY_SERVICE_FACTORY_H_

#include "base/memory/singleton.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/service_access_type.h"

namespace history {
class HistoryService;
}  // history

class HistoryServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  static history::HistoryService* GetForProfile(Profile* profile,
                                                ServiceAccessType sat);

  static history::HistoryService* GetForProfileIfExists(Profile* profile,
                                                        ServiceAccessType sat);

  static HistoryServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<HistoryServiceFactory>;

  HistoryServiceFactory();
  virtual ~HistoryServiceFactory();

  // BrowserContextKeyedServiceFactory:
  virtual KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
};

#endif  // CHROME_BROWSER_HISTORY_HISTORY_SERVICE_FACTORY_H_
