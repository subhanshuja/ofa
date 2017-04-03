// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/sync/sync_profile_manager.h"

#include <utility>

#include "chrome/browser/profiles/profile.h"
#include "mobile/common/sync/sync_profile.h"

SyncProfileManager::SyncProfileManager(
    std::unique_ptr<mobile::SyncProfile> profile)
    : profile_(std::move(profile)) {}

SyncProfileManager::~SyncProfileManager() {}

std::vector<Profile*> SyncProfileManager::GetLoadedProfiles() const {
  std::vector<Profile*> ret;
  ret.push_back(profile_.get());
  return ret;
}

Profile* SyncProfileManager::GetProfileByPath(
    const base::FilePath& path) const {
  DCHECK(profile_->GetPath() == path);
  return profile_.get();
}
