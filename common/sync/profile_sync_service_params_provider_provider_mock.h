// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_PROFILE_SYNC_SERVICE_PARAMS_PROVIDER_PROVIDER_MOCK_H_
#define COMMON_SYNC_PROFILE_SYNC_SERVICE_PARAMS_PROVIDER_PROVIDER_MOCK_H_

#include "common/sync/profile_sync_service_params_provider.h"

#include "common/oauth2/auth_service_mock.h"
#include "common/oauth2/session/persistent_session_mock.h"

namespace opera {

class ProfileSyncServiceParamsProviderProviderMock
    : public ProfileSyncServiceParamsProvider::Provider {
 public:
  ProfileSyncServiceParamsProviderProviderMock();
  ~ProfileSyncServiceParamsProviderProviderMock() override;

  void GetProfileSyncServiceInitParamsForProfile(
      Profile* profile,
      opera::OperaSyncInitParams* params) override;

  std::unique_ptr<oauth2::AuthServiceMock> auth_service_mock_;
  std::unique_ptr<oauth2::PersistentSessionMock> persistent_session_mock_;
};

#endif  // COMMON_SYNC_PROFILE_SYNC_SERVICE_PARAMS_PROVIDER_PROVIDER_MOCK_H_
}  // namespace opera
