// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/profile_sync_service_params_provider.h"

#include "base/logging.h"

namespace opera {

// static
ProfileSyncServiceParamsProvider*
ProfileSyncServiceParamsProvider::GetInstance() {
  return base::Singleton<ProfileSyncServiceParamsProvider>::get();
}

ProfileSyncServiceParamsProvider::ProfileSyncServiceParamsProvider()
    : provider_(NULL) {
}

ProfileSyncServiceParamsProvider::~ProfileSyncServiceParamsProvider() {
}

void ProfileSyncServiceParamsProvider::GetInitParamsForProfile(
    Profile* profile,
    OperaSyncInitParams* params) {
  DCHECK(provider_) << "Make sure to set provider early.";
  provider_->GetProfileSyncServiceInitParamsForProfile(profile, params);
}

}  // namespace opera
