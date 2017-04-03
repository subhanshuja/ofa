// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/network/revoke_token_request.h"

#include "net/base/load_flags.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "common/oauth2/network/revoke_token_response.h"
#include "common/oauth2/test/testing_constants.h"

namespace opera {
namespace oauth2 {
namespace {

using ::testing::_;
using ::testing::Return;

const int kDefaultLoadFlags = net::LOAD_DISABLE_CACHE |
                              net::LOAD_DO_NOT_SAVE_COOKIES |
                              net::LOAD_DO_NOT_SEND_COOKIES;
const std::string kApplicationXWwwFormUrlencoded =
    "application/x-www-form-urlencoded";

}  // namespace

TEST(RevokeTokenRequestTest, RevokeAccessTokenContents) {
  scoped_refptr<RevokeTokenRequest> request = RevokeTokenRequest::Create(
      RevokeTokenRequest::TT_ACCESS_TOKEN, test::kMockAccessToken,
      test::kMockClientId, test::kMockSessionId);
  EXPECT_EQ(
      "client_id=mock-client-id&token=mock-"
      "access-token&token_type_hint=access_token",
      request->GetContent());
  EXPECT_EQ(net::URLFetcher::POST, request->GetHTTPRequestType());
  EXPECT_EQ(kDefaultLoadFlags, request->GetLoadFlags());
  EXPECT_EQ("/oauth2/v1/revoketoken/", request->GetPath());
  EXPECT_EQ(kApplicationXWwwFormUrlencoded, request->GetRequestContentType());
  EXPECT_EQ("sid=mock-session-id", request->GetQueryString());
}

TEST(RevokeTokenRequestTest, RevokeRefreshTokenContents) {
  scoped_refptr<RevokeTokenRequest> request = RevokeTokenRequest::Create(
      RevokeTokenRequest::TT_REFRESH_TOKEN, test::kMockRefreshToken,
      test::kMockClientId, test::kMockSessionId);
  EXPECT_EQ(
      "client_id=mock-client-id&token=mock-"
      "refresh-token&token_type_hint=refresh_token",
      request->GetContent());
  EXPECT_EQ(net::URLFetcher::POST, request->GetHTTPRequestType());
  EXPECT_EQ(kDefaultLoadFlags, request->GetLoadFlags());
  EXPECT_EQ("/oauth2/v1/revoketoken/", request->GetPath());
  EXPECT_EQ(kApplicationXWwwFormUrlencoded, request->GetRequestContentType());
  EXPECT_EQ("sid=mock-session-id", request->GetQueryString());
}

TEST(RevokeTokenRequestTest, RevokeAccessTokenTryResponseHttpProblem) {
  scoped_refptr<RevokeTokenRequest> request = RevokeTokenRequest::Create(
      RevokeTokenRequest::TT_ACCESS_TOKEN, test::kMockAccessToken,
      test::kMockClientId, test::kMockSessionId);
  const std::set<int> kValidStatusCodes{200, 400, 401};

  for (int i = 100; i <= 600; i++) {
    if (kValidStatusCodes.find(i) != kValidStatusCodes.end()) {
      EXPECT_EQ(RS_PARSE_PROBLEM, request->TryResponse(i, "garbage OH MY!"))
          << "Wrong response for HTTP status " << i;
    } else {
      EXPECT_EQ(RS_HTTP_PROBLEM, request->TryResponse(i, "garbage OH MY!"))
          << "Wrong response for HTTP status " << i;
    }
    EXPECT_NE(nullptr, request->revoke_token_response());
    EXPECT_EQ("", request->revoke_token_response()->error_message());
    EXPECT_EQ(OAE_UNSET, request->revoke_token_response()->auth_error());
  }
}

TEST(RevokeTokenRequestTest, RevokeRefreshTokenTryResponseHttpProblem) {
  scoped_refptr<RevokeTokenRequest> request = RevokeTokenRequest::Create(
      RevokeTokenRequest::TT_REFRESH_TOKEN, test::kMockAccessToken,
      test::kMockClientId, test::kMockSessionId);
  const std::set<int> kValidStatusCodes{200, 400, 401};

  for (int i = 100; i <= 600; i++) {
    if (kValidStatusCodes.find(i) != kValidStatusCodes.end()) {
      EXPECT_EQ(RS_PARSE_PROBLEM, request->TryResponse(i, "garbage OH MY!"))
          << "Wrong response for HTTP status " << i;
    } else {
      EXPECT_EQ(RS_HTTP_PROBLEM, request->TryResponse(i, "garbage OH MY!"))
          << "Wrong response for HTTP status " << i;
    }
    EXPECT_NE(nullptr, request->revoke_token_response());
    EXPECT_EQ("", request->revoke_token_response()->error_message());
    EXPECT_EQ(OAE_UNSET, request->revoke_token_response()->auth_error());
  }
}

TEST(RevokeTokenRequestTest, RevokeAccessTokenTryResponse200) {
  scoped_refptr<RevokeTokenRequest> request = RevokeTokenRequest::Create(
      RevokeTokenRequest::TT_ACCESS_TOKEN, test::kMockAccessToken,
      test::kMockClientId, test::kMockSessionId);
  EXPECT_EQ(RS_OK, request->TryResponse(200, ""));
  EXPECT_NE(nullptr, request->revoke_token_response());
  EXPECT_EQ("", request->revoke_token_response()->error_message());
  EXPECT_EQ(OAE_UNSET, request->revoke_token_response()->auth_error());
}

TEST(RevokeTokenRequestTest, RevokeAccessTokenTryResponse400InvalidRequest) {
  scoped_refptr<RevokeTokenRequest> request = RevokeTokenRequest::Create(
      RevokeTokenRequest::TT_ACCESS_TOKEN, test::kMockAccessToken,
      test::kMockClientId, test::kMockSessionId);
  EXPECT_EQ(RS_OK, request->TryResponse(400,
                                        "{\"error\":\"invalid_request\", "
                                        "\"error_description\":\"whatever "
                                        "comes here\"}"));
  EXPECT_NE(nullptr, request->revoke_token_response());
  EXPECT_EQ("whatever comes here",
            request->revoke_token_response()->error_message());
  EXPECT_EQ(OAE_INVALID_REQUEST,
            request->revoke_token_response()->auth_error());
}

TEST(RevokeTokenRequestTest,
     RevokeAccessTokenTryResponse400InvalidRequestNoErrorDescription) {
  scoped_refptr<RevokeTokenRequest> request = RevokeTokenRequest::Create(
      RevokeTokenRequest::TT_ACCESS_TOKEN, test::kMockAccessToken,
      test::kMockClientId, test::kMockSessionId);
  EXPECT_EQ(RS_OK,
            request->TryResponse(400, "{\"error\":\"invalid_request\"}"));
  EXPECT_NE(nullptr, request->revoke_token_response());
  EXPECT_EQ("", request->revoke_token_response()->error_message());
  EXPECT_EQ(OAE_INVALID_REQUEST,
            request->revoke_token_response()->auth_error());
}

TEST(RevokeTokenRequestTest, RevokeAccessTokenTryResponse400InvalidClient) {
  scoped_refptr<RevokeTokenRequest> request = RevokeTokenRequest::Create(
      RevokeTokenRequest::TT_ACCESS_TOKEN, test::kMockAccessToken,
      test::kMockClientId, test::kMockSessionId);
  EXPECT_EQ(RS_PARSE_PROBLEM,
            request->TryResponse(400,
                                 "{\"error\":\"invalid_client\", "
                                 "\"error_description\":\"whatever comes "
                                 "here\"}"));
  EXPECT_NE(nullptr, request->revoke_token_response());
  EXPECT_EQ("", request->revoke_token_response()->error_message());
  EXPECT_EQ(OAE_UNSET, request->revoke_token_response()->auth_error());
}

TEST(RevokeTokenRequestTest, RevokeAccessTokenTryResponse401InvalidClient) {
  scoped_refptr<RevokeTokenRequest> request = RevokeTokenRequest::Create(
      RevokeTokenRequest::TT_ACCESS_TOKEN, test::kMockAccessToken,
      test::kMockClientId, test::kMockSessionId);
  EXPECT_EQ(RS_OK, request->TryResponse(401,
                                        "{\"error\":\"invalid_client\", "
                                        "\"error_description\":\"whatever "
                                        "comes here\"}"));
  EXPECT_NE(nullptr, request->revoke_token_response());
  EXPECT_EQ("whatever comes here",
            request->revoke_token_response()->error_message());
  EXPECT_EQ(OAE_INVALID_CLIENT, request->revoke_token_response()->auth_error());
}

TEST(RevokeTokenRequestTest,
     RevokeAccessTokenTryResponse401InvalidClientNoErrorDescription) {
  scoped_refptr<RevokeTokenRequest> request = RevokeTokenRequest::Create(
      RevokeTokenRequest::TT_ACCESS_TOKEN, test::kMockAccessToken,
      test::kMockClientId, test::kMockSessionId);
  EXPECT_EQ(RS_OK, request->TryResponse(401, "{\"error\":\"invalid_client\"}"));
  EXPECT_NE(nullptr, request->revoke_token_response());
  EXPECT_EQ("", request->revoke_token_response()->error_message());
  EXPECT_EQ(OAE_INVALID_CLIENT, request->revoke_token_response()->auth_error());
}

TEST(RevokeTokenRequestTest, RevokeAccessTokenTryResponse401InvalidRequest) {
  scoped_refptr<RevokeTokenRequest> request = RevokeTokenRequest::Create(
      RevokeTokenRequest::TT_ACCESS_TOKEN, test::kMockAccessToken,
      test::kMockClientId, test::kMockSessionId);
  EXPECT_EQ(
      RS_PARSE_PROBLEM,
      request->TryResponse(401,
                           "{\"error\":\"invalid_request\", "
                           "\"error_description\":\"whatever comes here\"}"));
  EXPECT_NE(nullptr, request->revoke_token_response());
  EXPECT_EQ("", request->revoke_token_response()->error_message());
  EXPECT_EQ(OAE_UNSET, request->revoke_token_response()->auth_error());
}

}  // namespace oauth2
}  // namespace opera
