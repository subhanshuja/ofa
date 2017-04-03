// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_SYNC_SYNC_PROFILE_MANAGER_H_
#define MOBILE_COMMON_SYNC_SYNC_PROFILE_MANAGER_H_

#include <vector>
#include <memory>

#include "chrome/browser/profiles/profile_manager.h"

namespace mobile {
class SyncProfile;
}

class SyncProfileManager : public ProfileManager {
 public:
  explicit SyncProfileManager(std::unique_ptr<mobile::SyncProfile> profile);
  virtual ~SyncProfileManager();

  std::vector<Profile*> GetLoadedProfiles() const override;
  Profile* GetProfileByPath(const base::FilePath& path) const override;

  mobile::SyncProfile* profile() { return profile_.get(); }

 private:
  const std::unique_ptr<mobile::SyncProfile> profile_;
};

#endif  // MOBILE_COMMON_SYNC_SYNC_PROFILE_MANAGER_H_
