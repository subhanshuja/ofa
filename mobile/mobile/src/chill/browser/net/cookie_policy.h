// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_NET_COOKIE_POLICY_H_
#define CHILL_BROWSER_NET_COOKIE_POLICY_H_

#include "base/memory/singleton.h"
#include "base/synchronization/lock.h"
#include "net/base/static_cookie_policy.h"

class GURL;

namespace opera {

class CookiePolicy {
 public:
  static CookiePolicy* GetInstance();

  void UpdateFromSettings();
  bool CanGetCookies(const GURL& url,
                     const GURL& first_party_for_cookies) const;
  bool CanSetCookie(const GURL& url,
                    const GURL& first_party_for_cookies) const;

 private:
  friend struct base::DefaultSingletonTraits<CookiePolicy>;
  CookiePolicy();

  // Used to guarantee thread-safe access to |policy_|.
  mutable base::Lock lock_;

  net::StaticCookiePolicy policy_;

  DISALLOW_COPY_AND_ASSIGN(CookiePolicy);
};

}  // namespace opera

#endif  // CHILL_BROWSER_NET_COOKIE_POLICY_H_
