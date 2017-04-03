// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/test/testing_constants.h"

namespace opera {
namespace oauth2 {
namespace test {

const std::string kMockAccessToken = "mock-access-token";
const std::string kMockAuthToken = "mock-auth-token";
const std::string kMockClientId = "mock-client-id";
const std::string kMockClientName1 = "mock-client-name1";
const std::string kMockClientName2 = "mock-client-name2";
const std::string kMockRefreshToken = "mock-refresh-token";
const std::string kMockScope1 = "mock-scope-value-1";
const std::string kMockScope2 = "mock-scope-value-2";
const std::string kMockScope3 = "mock-scope-value-3";
const std::string kMockScope4 = "mock-scope-value-4";
const std::string kMockSessionId = "mock-session-id";
const std::string kMockTokenValue1 = "mock-token-value-1";
const std::string kMockTokenValue2 = "mock-token-value-2";
const std::string kMockUsername = "mock-username";
const std::string kMockOtherUsername = "mock-other-username";

const ScopeSet kMockScopeSet1 =
    ScopeSet::FromEncoded(kMockScope1 + " " + kMockScope2);
const ScopeSet kMockScopeSet2 =
    ScopeSet::FromEncoded(kMockScope4 + " " + kMockScope3);
const base::Time kMockValidExpirationTime =
    base::Time::Now() + base::TimeDelta::FromHours(1);
const base::Time kMockExpiredExpirationTime =
    base::Time::Now() - base::TimeDelta::FromHours(1);

const scoped_refptr<AuthToken> kValidToken1 =
    new AuthToken(kMockClientName1,
                  kMockTokenValue1,
                  kMockScopeSet1,
                  kMockValidExpirationTime);
const scoped_refptr<AuthToken> kValidToken2 =
    new AuthToken(kMockClientName1,
                  kMockTokenValue2,
                  kMockScopeSet2,
                  kMockValidExpirationTime);
const scoped_refptr<AuthToken> kValidToken3 =
    new AuthToken(kMockClientName2,
                  kMockTokenValue2,
                  kMockScopeSet2,
                  kMockValidExpirationTime);
const scoped_refptr<AuthToken> kValidToken4 =
    new AuthToken(kMockClientName2,
                  kMockTokenValue1,
                  kMockScopeSet2,
                  kMockValidExpirationTime);
const scoped_refptr<AuthToken> kInvalidToken =
    new AuthToken(std::string(),
                  kMockTokenValue1,
                  kMockScopeSet1,
                  kMockValidExpirationTime);
const scoped_refptr<AuthToken> kExpiredToken =
    new AuthToken(kMockClientName1,
                  kMockTokenValue1,
                  kMockScopeSet2,
                  kMockExpiredExpirationTime);

}  // namespace test
}  // namespace oauth2
}  // namespace opera
