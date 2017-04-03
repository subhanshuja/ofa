// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_TEST_UTILS_H_
#define COMMON_SYNC_SYNC_TEST_UTILS_H_

#include "common/account/time_skew_resolver.h"
#include "common/sync/sync_login_data.h"

namespace opera {
struct AccountAuthData;
class AccountAuthDataFetcher;
struct OperaSyncInitParams;
}  // namespace opera

// Provide default constructed |login_data| for InitParams representing
// no initial login data.
std::unique_ptr<opera::OperaSyncInitParams> MockInitParams(
    const opera::SyncLoginData& login_data);

opera::AccountAuthDataFetcher* MockUpdaterFactory(
    const opera::AccountAuthData&);

std::unique_ptr<opera::TimeSkewResolver> MockTimeSkewResolver();

#endif  // COMMON_SYNC_SYNC_TEST_UTILS_H_
