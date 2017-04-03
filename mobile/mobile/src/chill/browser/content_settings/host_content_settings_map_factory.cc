// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/browser/content_settings/host_content_settings_map_factory.cc
// @final-synchronized

#include "chill/browser/content_settings/host_content_settings_map_factory.h"

#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"

#include "chill/browser/opera_browser_context.h"
#include "common/sync/sync.h"

HostContentSettingsMapFactory::HostContentSettingsMapFactory()
    : RefcountedBrowserContextKeyedServiceFactory(
        "HostContentSettingsMap",
        BrowserContextDependencyManager::GetInstance()) {
}

HostContentSettingsMapFactory::~HostContentSettingsMapFactory() {
}

// static
HostContentSettingsMap* HostContentSettingsMapFactory::GetForForBrowserContext(
    content::BrowserContext* context) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  return static_cast<HostContentSettingsMap*>(
      GetInstance()->GetServiceForBrowserContext(context, true).get());
}

// static
HostContentSettingsMapFactory* HostContentSettingsMapFactory::GetInstance() {
  return base::Singleton<HostContentSettingsMapFactory>::get();
}

scoped_refptr<RefcountedKeyedService>
    HostContentSettingsMapFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  Profile* profile = mobile::Sync::GetInstance()->GetProfile();
  bool off_the_record = context->IsOffTheRecord();

  scoped_refptr<HostContentSettingsMap> settings_map(
      new HostContentSettingsMap(profile->GetPrefs(), off_the_record, false));

  return settings_map;
}

content::BrowserContext* HostContentSettingsMapFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return context;
}
