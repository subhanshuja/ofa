// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sync_test_utils.h"

#include <memory>

#include "base/bind.h"
#include "common/account/account_auth_data.h"
#include "common/account/account_auth_data_fetcher_mock.h"
#include "common/account/time_skew_resolver_mock.h"
#include "common/sync/sync_login_data_store_mock.h"
#include "common/sync/sync_types.h"

using ::testing::Return;

opera::AccountAuthDataFetcher* MockUpdaterFactory(
    const opera::AccountAuthData&) {
  return new opera::AccountAuthDataFetcherMock;
}

std::unique_ptr<opera::TimeSkewResolver> MockTimeSkewResolver() {
  return std::unique_ptr<opera::TimeSkewResolver>(new opera::TimeSkewResolverMock);
}

std::unique_ptr<opera::OperaSyncInitParams> MockInitParams(
    const opera::SyncLoginData& login_data) {
  std::unique_ptr<opera::OperaSyncInitParams>
      params(new opera::OperaSyncInitParams);

  std::unique_ptr<opera::SyncLoginDataStoreMock> login_data_store(
      new opera::SyncLoginDataStoreMock);
  EXPECT_CALL(*login_data_store, LoadLoginData())
      .WillRepeatedly(Return(login_data));
  params->login_data_store.reset(login_data_store.release());

  params->auth_data_updater_factory = base::Bind(&MockUpdaterFactory);

  params->time_skew_resolver_factory = base::Bind(&MockTimeSkewResolver);

  return params;
}
