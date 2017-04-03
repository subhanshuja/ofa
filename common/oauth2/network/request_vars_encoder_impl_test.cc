// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "common/oauth2/network/request_vars_encoder_impl.h"

namespace opera {
namespace oauth2 {

using ::testing::_;
using ::testing::Return;

TEST(RequestVarsEncoderImplTest, EncodeFormEncodedEmpty) {
  RequestVarsEncoderImpl encoder;
  const auto expected = "";
  EXPECT_EQ(expected, encoder.EncodeFormEncoded());
}

TEST(RequestVarsEncoderImplTest, EncodeFormEncodedOne) {
  RequestVarsEncoderImpl encoder;
  encoder.AddVar("key", "value");
  const auto expected = "key=value";
  EXPECT_EQ(expected, encoder.EncodeFormEncoded());
}

TEST(RequestVarsEncoderImplTest, EncodeFormEncodedTwo) {
  RequestVarsEncoderImpl encoder;
  encoder.AddVar("key", "value");
  encoder.AddVar("key2", "value2");
  const auto expected = "key=value&key2=value2";
  EXPECT_EQ(expected, encoder.EncodeFormEncoded());
}

TEST(RequestVarsEncoderImplTest, EncodeFormEncodedDoesEncode) {
  RequestVarsEncoderImpl encoder;
  encoder.AddVar(" a k e y % nme==", "va l = u   ");
  const auto expected =
      "%20a%20k%20e%20y%20%25%20nme%3D%3D=va%20l%20%3D%20u%20%20%20";
  EXPECT_EQ(expected, encoder.EncodeFormEncoded());
}

TEST(RequestVarsEncoderImplTest, EncodeFormEncodedSemicolonEncoding) {
  RequestVarsEncoderImpl encoder;
  encoder.AddVar("scope", "http://mock.scope.opera.rynek.wroc.com.pl");
  const auto expected = "scope=http://mock.scope.opera.rynek.wroc.com.pl";
  EXPECT_EQ(expected, encoder.EncodeFormEncoded());
}

TEST(RequestVarsEncoderImplTest, EncodeQueryStringEmpty) {
  RequestVarsEncoderImpl encoder;
  const auto expected = "";
  EXPECT_EQ(expected, encoder.EncodeQueryString());
}

TEST(RequestVarsEncoderImplTest, EncodeQueryStringOne) {
  RequestVarsEncoderImpl encoder;
  encoder.AddVar("key", "value");
  const auto expected = "key=value";
  EXPECT_EQ(expected, encoder.EncodeQueryString());
}

TEST(RequestVarsEncoderImplTest, EncodeQueryStringTwo) {
  RequestVarsEncoderImpl encoder;
  encoder.AddVar("key", "value");
  encoder.AddVar("key2", "value2");
  const auto expected = "key=value&key2=value2";
  EXPECT_EQ(expected, encoder.EncodeQueryString());
}

TEST(RequestVarsEncoderImplTest, EncodeQueryStringDoesEncode) {
  RequestVarsEncoderImpl encoder;
  encoder.AddVar(" a k e y % nme==", "va l = u   ");
  const auto expected =
      "%20a%20k%20e%20y%20%25%20nme%3D%3D=va%20l%20%3D%20u%20%20%20";
  EXPECT_EQ(expected, encoder.EncodeQueryString());
}

TEST(RequestVarsEncoderImplTest, GetVarWorks) {
  RequestVarsEncoderImpl encoder;
  encoder.AddVar("key", "value");
  EXPECT_EQ("value", encoder.GetVar("key"));
}

TEST(RequestVarsEncoderImplTest, HasVarWorks) {
  RequestVarsEncoderImpl encoder;
  encoder.AddVar("key", "value");
  EXPECT_TRUE(encoder.HasVar("key"));
  EXPECT_FALSE(encoder.HasVar("badkey"));
}

}  // namespace oauth2
}  // namespace opera
