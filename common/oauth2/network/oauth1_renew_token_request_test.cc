// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/network/oauth1_renew_token_request.h"

#include <set>
#include <string>

#include "base/strings/string_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "common/oauth2/network/oauth1_renew_token_response.h"
#include "common/oauth2/util/scope_set.h"
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

const GURL kMockOAuth1BaseUrl("http://mock.oauth1.url/");
const GURL kMockOAuth2BaseUrl("http://mock.oauth2.url/");
const std::string kMockOAuth2ClientId = "mock_oauth2_client_id";
const std::string kMockOAuth2ClientSecret = "mock_oauth2_client_secret";

const std::string kMockOAuth1TokenSecret = "mock_oauth1_token_secret";
const ScopeSet kMockScopes({"mock-scope-1 mock-scope-2"});

const std::string kMockOAuth1ServiceId = "mock_oauth1_service_id";
const std::string kMockOAuth1Token = "mock_oauth1_token";
const std::string kMockOAuth1ClientId = "mock_oauth1_client_id";
const std::string kMockOAuth1ClientSecret = "mock_oauth1_client_secret";

}  // namespace

TEST(OAuth1RenewTokenTest, RequestContents) {
  scoped_refptr<OAuth1RenewTokenRequest> request =
      OAuth1RenewTokenRequest::Create(kMockOAuth1ServiceId, kMockOAuth1Token,
                                      kMockOAuth1ClientId,
                                      kMockOAuth1ClientSecret);
  EXPECT_EQ(std::string(), request->GetContent());
  EXPECT_EQ(net::URLFetcher::GET, request->GetHTTPRequestType());
  EXPECT_EQ(kDefaultLoadFlags, request->GetLoadFlags());
  EXPECT_EQ("/account/access-token/renewal/", request->GetPath());
  EXPECT_EQ(std::vector<std::string>{}, request->GetExtraRequestHeaders());
  EXPECT_EQ(
      "consumer_key=mock_oauth1_client_id&old_token=mock_oauth1_token&service="
      "mock_oauth1_service_id&signature="
      "a4cde0efc5af80173bb8c5b92ff8ad7366cf9d72",
      request->GetQueryString());
  EXPECT_EQ(RMUT_OAUTH1, request->GetRequestManagerUrlType());
  EXPECT_EQ(std::string(), request->GetRequestContentType());
}

TEST(OAuth1RenewTokenTest, TryResponseTokenRenewed) {
  scoped_refptr<OAuth1RenewTokenRequest> request =
      OAuth1RenewTokenRequest::Create(kMockOAuth1ServiceId, kMockOAuth1Token,
                                      kMockOAuth1ClientId,
                                      kMockOAuth1ClientSecret);
  EXPECT_EQ(RS_OK, request->TryResponse(
                       200,
                       "{\"auth_token\": \"mock-access-token\", "
                       "\"auth_token_secret\": \"mock-access-token-secret\", "
                       "\"userName\": \"mock-user-name\", \"userEmail\": "
                       "\"mock-user-email\"}"));
  EXPECT_NE(nullptr, request->oauth1_renew_token_response());
  EXPECT_EQ(-1, request->oauth1_renew_token_response()->oauth1_error_code());
  EXPECT_EQ(std::string(),
            request->oauth1_renew_token_response()->oauth1_error_message());
  EXPECT_EQ("mock-access-token",
            request->oauth1_renew_token_response()->oauth1_auth_token());
  EXPECT_EQ("mock-access-token-secret",
            request->oauth1_renew_token_response()->oauth1_auth_token_secret());
}

TEST(OAuth1RenewTokenTest, TryResponseTokenRenewedNoUserNameNoUserEmail) {
  scoped_refptr<OAuth1RenewTokenRequest> request =
      OAuth1RenewTokenRequest::Create(kMockOAuth1ServiceId, kMockOAuth1Token,
                                      kMockOAuth1ClientId,
                                      kMockOAuth1ClientSecret);
  EXPECT_EQ(
      RS_OK,
      request->TryResponse(
          200,
          "{\"auth_token\": \"mock-access-token\", "
          "\"auth_token_secret\": "
          "\"mock-access-token-secret\", \"userName\": \"mock-user-name\"}"));
  EXPECT_NE(nullptr, request->oauth1_renew_token_response());
  EXPECT_EQ(-1, request->oauth1_renew_token_response()->oauth1_error_code());
  EXPECT_EQ(std::string(),
            request->oauth1_renew_token_response()->oauth1_error_message());
  EXPECT_EQ("mock-access-token",
            request->oauth1_renew_token_response()->oauth1_auth_token());
  EXPECT_EQ("mock-access-token-secret",
            request->oauth1_renew_token_response()->oauth1_auth_token_secret());
}

TEST(OAuth1RenewTokenTest, TryResponseTokenRenewedNoUserName) {
  scoped_refptr<OAuth1RenewTokenRequest> request =
      OAuth1RenewTokenRequest::Create(kMockOAuth1ServiceId, kMockOAuth1Token,
                                      kMockOAuth1ClientId,
                                      kMockOAuth1ClientSecret);
  EXPECT_EQ(RS_OK, request->TryResponse(
                       200,
                       "{\"auth_token\": \"mock-access-token\", "
                       "\"auth_token_secret\": \"mock-access-token-secret\", "
                       "\"userEmail\": \"mock-user-email\"}"));
  EXPECT_NE(nullptr, request->oauth1_renew_token_response());
  EXPECT_EQ(-1, request->oauth1_renew_token_response()->oauth1_error_code());
  EXPECT_EQ(std::string(),
            request->oauth1_renew_token_response()->oauth1_error_message());
  EXPECT_EQ("mock-access-token",
            request->oauth1_renew_token_response()->oauth1_auth_token());
  EXPECT_EQ("mock-access-token-secret",
            request->oauth1_renew_token_response()->oauth1_auth_token_secret());
}

TEST(OAuth1RenewTokenTest, TryResponseTokenRenewedNoUserEmail) {
  scoped_refptr<OAuth1RenewTokenRequest> request =
      OAuth1RenewTokenRequest::Create(kMockOAuth1ServiceId, kMockOAuth1Token,
                                      kMockOAuth1ClientId,
                                      kMockOAuth1ClientSecret);
  EXPECT_EQ(RS_OK, request->TryResponse(
                       200,
                       "{\"auth_token\": \"mock-access-token\", "
                       "\"auth_token_secret\": \"mock-access-token-secret\", "
                       "\"userName\": \"mock-user-name\"}"));
  EXPECT_NE(nullptr, request->oauth1_renew_token_response());
  EXPECT_EQ(-1, request->oauth1_renew_token_response()->oauth1_error_code());
  EXPECT_EQ(std::string(),
            request->oauth1_renew_token_response()->oauth1_error_message());
  EXPECT_EQ("mock-access-token",
            request->oauth1_renew_token_response()->oauth1_auth_token());
  EXPECT_EQ("mock-access-token-secret",
            request->oauth1_renew_token_response()->oauth1_auth_token_secret());
}

TEST(OAuth1RenewTokenTest, TryResponseTokenRenewalError) {
  scoped_refptr<OAuth1RenewTokenRequest> request =
      OAuth1RenewTokenRequest::Create(kMockOAuth1ServiceId, kMockOAuth1Token,
                                      kMockOAuth1ClientId,
                                      kMockOAuth1ClientSecret);
  EXPECT_EQ(RS_OK,
            request->TryResponse(200,
                                 "{\"err_code\": 666, \"err_msg\": \"The "
                                 "auth.opera.com server is broken\"}"));
  EXPECT_NE(nullptr, request->oauth1_renew_token_response());
  EXPECT_EQ(666, request->oauth1_renew_token_response()->oauth1_error_code());
  EXPECT_EQ("The auth.opera.com server is broken",
            request->oauth1_renew_token_response()->oauth1_error_message());
  EXPECT_EQ(std::string(),
            request->oauth1_renew_token_response()->oauth1_auth_token());
  EXPECT_EQ(std::string(),
            request->oauth1_renew_token_response()->oauth1_auth_token_secret());
}

TEST(OAuth1RenewTokenTest, TryResponseHttpProblem) {
  scoped_refptr<OAuth1RenewTokenRequest> request =
      OAuth1RenewTokenRequest::Create(kMockOAuth1ServiceId, kMockOAuth1Token,
                                      kMockOAuth1ClientId,
                                      kMockOAuth1ClientSecret);
  const std::set<int> kValidStatusCodes{200};

  for (int i = 100; i <= 600; i++) {
    if (kValidStatusCodes.find(i) != kValidStatusCodes.end()) {
      EXPECT_EQ(RS_PARSE_PROBLEM, request->TryResponse(i, "garbage OH MY!"))
          << "Wrong response for HTTP status " << i;
    } else {
      EXPECT_EQ(RS_HTTP_PROBLEM, request->TryResponse(i, "garbage OH MY!"))
          << "Wrong response for HTTP status " << i;
    }
    EXPECT_NE(nullptr, request->oauth1_renew_token_response());
    EXPECT_EQ(-1, request->oauth1_renew_token_response()->oauth1_error_code());
    EXPECT_EQ(std::string(),
              request->oauth1_renew_token_response()->oauth1_error_message());
    EXPECT_EQ(std::string(),
              request->oauth1_renew_token_response()->oauth1_auth_token());
    EXPECT_EQ(
        std::string(),
        request->oauth1_renew_token_response()->oauth1_auth_token_secret());
  }
}

}  // namespace oauth2
}  // namespace opera
