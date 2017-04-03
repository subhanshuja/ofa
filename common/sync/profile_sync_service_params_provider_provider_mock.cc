// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/profile_sync_service_params_provider_provider_mock.h"

#include "base/bind.h"
#include "testing/gmock/include/gmock/gmock.h"

#include "common/oauth2/auth_service_mock.h"
#include "common/sync/sync_login_data_store_mock.h"
#include "common/sync/sync_test_utils.h"

using ::testing::Return;

namespace opera {
ProfileSyncServiceParamsProviderProviderMock::
    ProfileSyncServiceParamsProviderProviderMock() {}

ProfileSyncServiceParamsProviderProviderMock::
    ~ProfileSyncServiceParamsProviderProviderMock() {}

void ProfileSyncServiceParamsProviderProviderMock::
    GetProfileSyncServiceInitParamsForProfile(
        Profile* profile,
        opera::OperaSyncInitParams* params) {
  DCHECK(params);
  std::unique_ptr<opera::SyncLoginDataStoreMock> login_data_store(
      new opera::SyncLoginDataStoreMock);
  ON_CALL(*login_data_store, LoadLoginData())
      .WillByDefault(Return(opera::SyncLoginData()));
  params->login_data_store.reset(login_data_store.release());
  params->auth_data_updater_factory = base::Bind(&MockUpdaterFactory);

  // Note: We should track the auth service per profile here, should this be
  // used in multiprofile tests.
  persistent_session_mock_.reset(new oauth2::PersistentSessionMock);
  ON_CALL(*persistent_session_mock_, GetState())
      .WillByDefault(Return(oauth2::OA2SS_INACTIVE));

  auth_service_mock_.reset(new oauth2::AuthServiceMock);
  ON_CALL(*auth_service_mock_, GetSession())
      .WillByDefault(Return(persistent_session_mock_.get()));

  params->auth_service = auth_service_mock_.get();
}

}  // namespace opera
