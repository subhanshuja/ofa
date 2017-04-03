// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_TOKEN_CACHE_TOKEN_CACHE_TEST_UTIL_H_
#define COMMON_OAUTH2_TOKEN_CACHE_TOKEN_CACHE_TEST_UTIL_H_

#include <map>
#include <string>

#include "base/memory/ref_counted.h"

#include "common/oauth2/util/token.h"
#include "common/oauth2/util/util.h"

namespace opera {
namespace oauth2 {
namespace test {

extern const std::string kMockAccessToken;
extern const std::string kMockAuthToken;
extern const std::string kMockClientId;
extern const std::string kMockClientName1;
extern const std::string kMockClientName2;
extern const std::string kMockRefreshToken;
extern const std::string kMockScope1;
extern const std::string kMockScope2;
extern const std::string kMockScope3;
extern const std::string kMockScope4;
extern const std::string kMockSessionId;
extern const std::string kMockTokenValue1;
extern const std::string kMockTokenValue2;
extern const std::string kMockUsername;
extern const std::string kMockOtherUsername;
extern const ScopeSet kMockScopeSet1;
extern const ScopeSet kMockScopeSet2;
extern const base::Time kMockValidExpirationTime;
extern const base::Time kMockExpiredExpirationTime;
extern const scoped_refptr<AuthToken> kValidToken1;
extern const scoped_refptr<AuthToken> kValidToken2;
extern const scoped_refptr<AuthToken> kValidToken3;
extern const scoped_refptr<AuthToken> kValidToken4;
extern const scoped_refptr<AuthToken> kInvalidToken;
extern const scoped_refptr<AuthToken> kExpiredToken;

}  // namespace test
}  // namespace oauth2
}  // namespace opera

#endif  // COMMON_OAUTH2_TOKEN_CACHE_TOKEN_CACHE_TEST_UTIL_H_
