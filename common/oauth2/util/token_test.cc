// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/util/token.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace opera {
namespace oauth2 {
namespace {

using ::testing::_;

const std::string kMockClientName = "mock-client-name";
const std::string kMockToken = "mock-token";
const ScopeSet kMockScopes = ScopeSet::FromEncoded("mock-scope");

class TokenTest : public ::testing::Test {
 public:
  TokenTest() {
    EXPECT_TRUE(base::Time::FromString("Tue, 15 Nov 1994 12:45:26 GMT",
                                       &kMockPastExpirationTime));
    kMockFutureExpirationTime =
        base::Time::Now() + base::TimeDelta::FromDays(1);
  }
  ~TokenTest() override {}

 protected:
  base::Time kMockPastExpirationTime;
  base::Time kMockFutureExpirationTime;
};
}  // namespace

TEST_F(TokenTest, Smoke) {
  scoped_refptr<AuthToken> token = make_scoped_refptr<AuthToken>(new AuthToken(
      kMockClientName, kMockToken, kMockScopes, kMockFutureExpirationTime));
  EXPECT_EQ(kMockClientName, token->client_name());
  EXPECT_EQ(kMockToken, token->token());
  EXPECT_EQ(kMockScopes, token->scopes());
  EXPECT_EQ(kMockFutureExpirationTime, token->expiration_time());
  EXPECT_TRUE(token->is_valid());
  EXPECT_FALSE(token->is_expired());
}

TEST_F(TokenTest, InvalidEmptyClientName) {
  scoped_refptr<AuthToken> token = make_scoped_refptr<AuthToken>(
      new AuthToken("", kMockToken, kMockScopes, kMockFutureExpirationTime));
  EXPECT_FALSE(token->is_valid());
  EXPECT_FALSE(token->is_expired());
}

TEST_F(TokenTest, InvalidEmptyToken) {
  scoped_refptr<AuthToken> token = make_scoped_refptr<AuthToken>(new AuthToken(
      kMockClientName, "", kMockScopes, kMockFutureExpirationTime));
  EXPECT_FALSE(token->is_valid());
  EXPECT_FALSE(token->is_expired());
}

TEST_F(TokenTest, InvalidEmptyScopes) {
  scoped_refptr<AuthToken> token = make_scoped_refptr<AuthToken>(
      new AuthToken(kMockClientName, kMockToken, ScopeSet(),
                    kMockFutureExpirationTime));
  EXPECT_FALSE(token->is_valid());
  EXPECT_FALSE(token->is_expired());
}

TEST_F(TokenTest, InvalidEmptyExpirationTime) {
  scoped_refptr<AuthToken> token = make_scoped_refptr<AuthToken>(
      new AuthToken(kMockClientName, kMockToken, kMockScopes, base::Time()));
  EXPECT_FALSE(token->is_valid());
  EXPECT_TRUE(token->is_expired());
}

TEST_F(TokenTest, NonExpired) {
  scoped_refptr<AuthToken> token = make_scoped_refptr<AuthToken>(new AuthToken(
      kMockClientName, kMockToken, kMockScopes, kMockFutureExpirationTime));
  EXPECT_TRUE(token->is_valid());
  EXPECT_FALSE(token->is_expired());
}

TEST_F(TokenTest, Expired) {
  scoped_refptr<AuthToken> token = make_scoped_refptr<AuthToken>(new AuthToken(
      kMockClientName, kMockToken, kMockScopes, kMockPastExpirationTime));
  EXPECT_TRUE(token->is_valid());
  EXPECT_TRUE(token->is_expired());
}

TEST_F(TokenTest, ComparisonOperator) {
  scoped_refptr<AuthToken> token1 = make_scoped_refptr<AuthToken>(new AuthToken(
      kMockClientName, kMockToken, kMockScopes, kMockFutureExpirationTime));
  scoped_refptr<AuthToken> token2 = make_scoped_refptr<AuthToken>(new AuthToken(
      kMockClientName, kMockToken, kMockScopes, kMockFutureExpirationTime));
  EXPECT_EQ(*token1, *token2);
}

}  // namespace oauth2
}  // namespace opera
