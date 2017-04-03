// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/net/cookie_policy.h"

#include "base/logging.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/net_errors.h"

#include "common/settings/settings_manager.h"

namespace opera {

CookiePolicy::CookiePolicy()
    : policy_(net::StaticCookiePolicy::ALLOW_ALL_COOKIES) {
}

CookiePolicy* CookiePolicy::GetInstance() {
  return base::Singleton<CookiePolicy>::get();
}

void CookiePolicy::UpdateFromSettings() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));

  base::AutoLock auto_lock(lock_);

  switch (SettingsManager::GetCookies()) {
    case SettingsManager::DISABLED:
      policy_.set_type(net::StaticCookiePolicy::BLOCK_ALL_COOKIES);
      break;
    case SettingsManager::ENABLED:
      policy_.set_type(net::StaticCookiePolicy::ALLOW_ALL_COOKIES);
      break;
    case SettingsManager::ENABLED_NO_THIRD_PARTY:
      policy_.set_type(net::StaticCookiePolicy::BLOCK_ALL_THIRD_PARTY_COOKIES);
      break;
    default:
      LOG(WARNING) << "Unknown cookies setting reported from settings manager.";
      DCHECK(false);
      break;
  }
}

bool CookiePolicy::CanGetCookies(
    const GURL& url, const GURL& first_party_for_cookies) const {
  base::AutoLock auto_lock(lock_);
  return policy_.CanGetCookies(url, first_party_for_cookies) == net::OK;
}

bool CookiePolicy::CanSetCookie(
    const GURL& url, const GURL& first_party_for_cookies) const {
  base::AutoLock auto_lock(lock_);
  return policy_.CanSetCookie(url, first_party_for_cookies) == net::OK;
}

}  // namespace opera
