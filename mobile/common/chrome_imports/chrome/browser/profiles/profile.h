// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PROFILES_PROFILE_H_
#define CHROME_BROWSER_PROFILES_PROFILE_H_

#include <string>

#include "base/sequenced_task_runner.h"
#include "content/public/browser/browser_context.h"

class PrefService;

namespace net {
class URLRequestContextGetter;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

class Profile : public content::BrowserContext {
public:
  enum ServiceAccessType {
    EXPLICIT_ACCESS,
    IMPLICIT_ACCESS
  };

  Profile();
  virtual ~Profile();

  // Returns the profile corresponding to the given browser context.
  static Profile* FromBrowserContext(content::BrowserContext* browser_context);

  virtual scoped_refptr<base::SequencedTaskRunner> GetIOTaskRunner() = 0;

  virtual PrefService* GetPrefs() = 0;

  virtual net::URLRequestContextGetter* GetRequestContext() = 0;

  virtual bool IsSupervised() = 0;

  std::string GetDebugName();

  bool IsSyncAllowed();

private:
  DISALLOW_COPY_AND_ASSIGN(Profile);
};

#endif  // CHROME_BROWSER_PROFILES_PROFILE_H_
