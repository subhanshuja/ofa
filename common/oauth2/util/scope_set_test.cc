// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "common/oauth2/util/util.h"
#include "common/oauth2/test/testing_constants.h"

namespace opera {
namespace oauth2 {

using ::testing::_;
using ::testing::Return;

TEST(ScopeSetTest, Empty) {
  ScopeSet scopes;
  EXPECT_EQ(0u, scopes.size());
  EXPECT_TRUE(scopes.empty());
  EXPECT_EQ("", scopes.encode());
  EXPECT_EQ(ScopeSet(), scopes);
}

TEST(ScopeSetTest, OneScope) {
  ScopeSet scopes = ScopeSet::FromEncoded(test::kMockScope1);
  EXPECT_EQ(1u, scopes.size());
  EXPECT_FALSE(scopes.empty());
  EXPECT_EQ(test::kMockScope1, scopes.encode());
  EXPECT_TRUE(scopes.has_scope(test::kMockScope1));
  EXPECT_FALSE(scopes.has_scope(test::kMockScope2));
}

TEST(ScopeSetTest, TwoScopes) {
  const std::set<std::string> two_scopes{test::kMockScope1, test::kMockScope2};
  ScopeSet scopes(two_scopes);
  EXPECT_EQ(2u, scopes.size());
  EXPECT_FALSE(scopes.empty());
  EXPECT_EQ(test::kMockScope1 + " " + test::kMockScope2, scopes.encode());
  EXPECT_TRUE(scopes.has_scope(test::kMockScope1));
  EXPECT_TRUE(scopes.has_scope(test::kMockScope2));
}

TEST(ScopeSetTest, DecodeOneScope) {
  const std::string& encoded = test::kMockScope1;
  ScopeSet scopes;
  scopes.decode(encoded);
  EXPECT_EQ(1u, scopes.size());
  EXPECT_FALSE(scopes.empty());
  EXPECT_EQ(test::kMockScope1, scopes.encode());
  EXPECT_TRUE(scopes.has_scope(test::kMockScope1));
  EXPECT_FALSE(scopes.has_scope(test::kMockScope2));
  EXPECT_EQ(ScopeSet::FromEncoded(test::kMockScope1), scopes);
}

TEST(ScopeSetTest, DecodeTwoScopes) {
  const std::string& encoded = test::kMockScope1 + " " + test::kMockScope2;
  ScopeSet scopes;
  scopes.decode(encoded);
  EXPECT_EQ(2u, scopes.size());
  EXPECT_FALSE(scopes.empty());
  EXPECT_EQ(test::kMockScope1 + " " + test::kMockScope2, scopes.encode());
  EXPECT_TRUE(scopes.has_scope(test::kMockScope1));
  EXPECT_TRUE(scopes.has_scope(test::kMockScope2));
  EXPECT_EQ(
      ScopeSet(std::set<std::string>{test::kMockScope1, test::kMockScope2}),
      scopes);
}

}  // namespace oauth2
}  // namespace opera
