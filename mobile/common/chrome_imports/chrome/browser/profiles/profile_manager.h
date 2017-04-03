// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PROFILES_PROFILE_MANAGER_H_
#define CHROME_BROWSER_PROFILES_PROFILE_MANAGER_H_

#include "base/files/file_path.h"
#include "base/threading/non_thread_safe.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/notification_observer.h"

class Profile;

class ProfileManager : public base::NonThreadSafe {
 public:
  virtual ~ProfileManager() {}

  virtual std::vector<Profile*> GetLoadedProfiles() const = 0;

  virtual Profile* GetProfileByPath(const base::FilePath& path) const = 0;
  static Profile* GetLastUsedProfile() {
    return NULL;
  }

 protected:
  ProfileManager() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(ProfileManager);
};

#endif  // CHROME_BROWSER_PROFILES_PROFILE_MANAGER_H_
