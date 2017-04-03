// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/network/migration_token_request.h"

#include <set>

#include "base/strings/string_split.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "net/base/load_flags.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "common/oauth2/device_name/device_name_service_mock.h"
#include "common/oauth2/network/migration_token_response.h"
#include "common/oauth2/test/testing_constants.h"
#include "common/oauth2/util/scope_set.h"

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

const std::string kMockOAuth1BaseHost = "mock.oauth1.url";
const GURL kMockOAuth1BaseUrl("http://" + kMockOAuth1BaseHost + "/");
const GURL kMockOAuth2BaseUrl("http://mock.oauth2.url/");
const std::string kMockOAuth1ClientId = "mock_oauth1_client_id";
const std::string kMockOAuth1ClientSecret = "mock_oauth1_client_secret";
const std::string kMockOAuth2ClientId = "mock_oauth2_client_id";
const base::TimeDelta kMockTimeSkew = base::TimeDelta::FromSeconds(-301);
const std::string kMockOAuth1Token = "mock_oauth1_token";
const std::string kMockOAuth1TokenSecret = "mock_oauth1_token_secret";
const ScopeSet kMockScopes({"mock-scope-1 mock-scope-2"});
const std::string kMockDeviceName = "mock_device_name";

}  // namespace

TEST(MigrationTokenRequestTest, RequestContents) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<MigrationTokenRequest> request = MigrationTokenRequest::Create(
      kMockOAuth1BaseUrl, kMockOAuth1ClientId, kMockOAuth1ClientSecret,
      kMockTimeSkew, kMockOAuth1Token, kMockOAuth1TokenSecret, kMockScopes,
      kMockOAuth2BaseUrl, kMockOAuth2ClientId, &device_name_service_mock,
      test::kMockSessionId);
  EXPECT_EQ(
      "client_id=mock_oauth2_client_id&"
      "grant_type=oauth1_token&scope=mock-scope-1%20mock-scope-2",
      request->GetContent());
  EXPECT_EQ(net::URLFetcher::POST, request->GetHTTPRequestType());
  EXPECT_EQ(kDefaultLoadFlags, request->GetLoadFlags());
  EXPECT_EQ("/oauth2/v1/token/", request->GetPath());

  const auto extra_request_headers1 = request->GetExtraRequestHeaders();
  EXPECT_EQ(1u, extra_request_headers1.size());
  const auto extra_header_1 = extra_request_headers1.at(0);
  auto tokens = base::SplitString(extra_header_1, ":", base::TRIM_WHITESPACE,
                                  base::SPLIT_WANT_NONEMPTY);
  ASSERT_EQ(2u, tokens.size());
  EXPECT_EQ(tokens.at(0), "Authorization");
  auto header_value = tokens.at(1).substr(6);
  tokens = base::SplitString(header_value, ",", base::TRIM_WHITESPACE,
                             base::SPLIT_WANT_NONEMPTY);
  ASSERT_EQ(9u, tokens.size());
  std::map<std::string, std::string> authorization_header_dissected;
  for (const auto token : tokens) {
    const auto key_value = base::SplitString(token, "=", base::TRIM_WHITESPACE,
                                             base::SPLIT_WANT_NONEMPTY);
    ASSERT_EQ(2u, key_value.size());
    const auto key = key_value.at(0);
    const auto value = key_value.at(1);
    authorization_header_dissected[key] =
        base::TrimString(value, "\"", base::TRIM_ALL).as_string();
  }
  EXPECT_EQ(1u, authorization_header_dissected.count("realm"));
  EXPECT_EQ(1u, authorization_header_dissected.count("oauth_consumer_key"));
  EXPECT_EQ(1u, authorization_header_dissected.count("oauth_nonce"));
  EXPECT_EQ(1u, authorization_header_dissected.count("oauth_signature"));
  EXPECT_EQ(1u, authorization_header_dissected.count("oauth_signature_method"));
  EXPECT_EQ(1u, authorization_header_dissected.count("oauth_timestamp"));
  EXPECT_EQ(1u, authorization_header_dissected.count("oauth_token"));
  EXPECT_EQ(1u, authorization_header_dissected.count("oauth_version"));
  EXPECT_EQ(1u, authorization_header_dissected.count("sid"));

  EXPECT_EQ(kMockOAuth1BaseHost, authorization_header_dissected.at("realm"));
  EXPECT_EQ(kMockOAuth1ClientId,
            authorization_header_dissected.at("oauth_consumer_key"));
  EXPECT_EQ("HMAC-SHA1",
            authorization_header_dissected.at("oauth_signature_method"));
  EXPECT_EQ(kMockOAuth1Token, authorization_header_dissected.at("oauth_token"));
  EXPECT_EQ("1.0", authorization_header_dissected.at("oauth_version"));
  EXPECT_EQ(test::kMockSessionId, authorization_header_dissected.at("sid"));

  // Check that the header is regenerated with each connection attempt.
  const auto extra_request_headers2 = request->GetExtraRequestHeaders();
  EXPECT_EQ(1u, extra_request_headers2.size());
  EXPECT_TRUE(base::StartsWith(extra_request_headers2.at(0), "Authorization:",
                               base::CompareCase::SENSITIVE));
  EXPECT_NE(extra_request_headers1, extra_request_headers2);
  EXPECT_EQ("sid=mock-session-id", request->GetQueryString());
  EXPECT_EQ(RMUT_OAUTH2, request->GetRequestManagerUrlType());
  EXPECT_EQ(kApplicationXWwwFormUrlencoded, request->GetRequestContentType());
}

TEST(MigrationTokenRequestTest, RequestContentsDeviceNameChanged) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(true));
  EXPECT_CALL(device_name_service_mock, GetCurrentDeviceName())
      .WillOnce(Return(kMockDeviceName));
  scoped_refptr<MigrationTokenRequest> request = MigrationTokenRequest::Create(
      kMockOAuth1BaseUrl, kMockOAuth1ClientId, kMockOAuth1ClientSecret,
      kMockTimeSkew, kMockOAuth1Token, kMockOAuth1TokenSecret, kMockScopes,
      kMockOAuth2BaseUrl, kMockOAuth2ClientId, &device_name_service_mock,
      test::kMockSessionId);
  EXPECT_EQ(
      "client_id=mock_oauth2_client_id&"
      "device_name=mock_device_name&"
      "grant_type=oauth1_token&scope=mock-scope-1%20mock-scope-2",
      request->GetContent());
  EXPECT_EQ(net::URLFetcher::POST, request->GetHTTPRequestType());
  EXPECT_EQ(kDefaultLoadFlags, request->GetLoadFlags());
  EXPECT_EQ("/oauth2/v1/token/", request->GetPath());
  const auto extra_request_headers1 = request->GetExtraRequestHeaders();
  EXPECT_EQ(1u, extra_request_headers1.size());
  EXPECT_TRUE(base::StartsWith(extra_request_headers1.at(0), "Authorization:",
                               base::CompareCase::SENSITIVE));
  const auto extra_request_headers2 = request->GetExtraRequestHeaders();
  EXPECT_EQ(1u, extra_request_headers2.size());
  EXPECT_TRUE(base::StartsWith(extra_request_headers2.at(0), "Authorization:",
                               base::CompareCase::SENSITIVE));
  EXPECT_NE(extra_request_headers1, extra_request_headers2);
  EXPECT_EQ("sid=mock-session-id", request->GetQueryString());
  EXPECT_EQ(RMUT_OAUTH2, request->GetRequestManagerUrlType());
  EXPECT_EQ(kApplicationXWwwFormUrlencoded, request->GetRequestContentType());
}

TEST(AccessTokenRequestTest, StoresCurrentDeviceNameOnSuccess) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(true));
  EXPECT_CALL(device_name_service_mock, GetCurrentDeviceName())
      .WillOnce(Return(kMockDeviceName));
  EXPECT_CALL(device_name_service_mock, StoreDeviceName(kMockDeviceName))
      .Times(1);
  scoped_refptr<MigrationTokenRequest> request = MigrationTokenRequest::Create(
      kMockOAuth1BaseUrl, kMockOAuth1ClientId, kMockOAuth1ClientSecret,
      kMockTimeSkew, kMockOAuth1Token, kMockOAuth1TokenSecret, kMockScopes,
      kMockOAuth2BaseUrl, kMockOAuth2ClientId, &device_name_service_mock,
      test::kMockSessionId);
  EXPECT_EQ(
      RS_OK,
      request->TryResponse(
          200,
          "{\"access_token\": \"mock-access-token\", "
          "\"refresh_token\": \"mock-refresh-token\", "
          "\"token_type\": \"Bearer\", \"expires_in\": "
          "3601, \"scope\": \"mock-granted-scopes\", \"user_id\": \"1298\"}"));
}

TEST(AccessTokenRequestTest, DoesntStoreCurrentDeviceNameOnHttpProblem) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(true));
  EXPECT_CALL(device_name_service_mock, GetCurrentDeviceName())
      .WillOnce(Return(kMockDeviceName));
  EXPECT_CALL(device_name_service_mock, StoreDeviceName(_)).Times(0);
  scoped_refptr<MigrationTokenRequest> request = MigrationTokenRequest::Create(
      kMockOAuth1BaseUrl, kMockOAuth1ClientId, kMockOAuth1ClientSecret,
      kMockTimeSkew, kMockOAuth1Token, kMockOAuth1TokenSecret, kMockScopes,
      kMockOAuth2BaseUrl, kMockOAuth2ClientId, &device_name_service_mock,
      test::kMockSessionId);
  EXPECT_EQ(
      RS_HTTP_PROBLEM,
      request->TryResponse(
          1,
          "{\"access_token\": \"mock-access-token\", "
          "\"refresh_token\": \"mock-refresh-token\", "
          "\"token_type\": \"Bearer\", \"expires_in\": "
          "3601, \"scope\": \"mock-granted-scopes\", \"user_id\": \"1298\"}"));
}

TEST(AccessTokenRequestTest, DoesntStoreCurrentDeviceNameOnParseProblem) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(true));
  EXPECT_CALL(device_name_service_mock, GetCurrentDeviceName())
      .WillOnce(Return(kMockDeviceName));
  EXPECT_CALL(device_name_service_mock, StoreDeviceName(_)).Times(0);
  scoped_refptr<MigrationTokenRequest> request = MigrationTokenRequest::Create(
      kMockOAuth1BaseUrl, kMockOAuth1ClientId, kMockOAuth1ClientSecret,
      kMockTimeSkew, kMockOAuth1Token, kMockOAuth1TokenSecret, kMockScopes,
      kMockOAuth2BaseUrl, kMockOAuth2ClientId, &device_name_service_mock,
      test::kMockSessionId);
  EXPECT_EQ(
      RS_PARSE_PROBLEM,
      request->TryResponse(
          200,
          "{\"wrong_access_token\": \"mock-access-token\", "
          "\"refresh_token\": \"mock-refresh-token\", "
          "\"token_type\": \"Bearer\", \"expires_in\": "
          "3601, \"scope\": \"mock-granted-scopes\", \"user_id\": \"1298\"}"));
}

TEST(AccessTokenRequestTest, DoesntStoreCurrentDeviceNameOnAuthError) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(true));
  EXPECT_CALL(device_name_service_mock, GetCurrentDeviceName())
      .WillOnce(Return(kMockDeviceName));
  EXPECT_CALL(device_name_service_mock, StoreDeviceName(_)).Times(0);
  scoped_refptr<MigrationTokenRequest> request = MigrationTokenRequest::Create(
      kMockOAuth1BaseUrl, kMockOAuth1ClientId, kMockOAuth1ClientSecret,
      kMockTimeSkew, kMockOAuth1Token, kMockOAuth1TokenSecret, kMockScopes,
      kMockOAuth2BaseUrl, kMockOAuth2ClientId, &device_name_service_mock,
      test::kMockSessionId);
  EXPECT_EQ(RS_OK, request->TryResponse(401, "{\"error\": \"invalid_grant\"}"));
}

TEST(AccessTokenRequestTest, TryResponseTokenGranted) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<MigrationTokenRequest> request = MigrationTokenRequest::Create(
      kMockOAuth1BaseUrl, kMockOAuth1ClientId, kMockOAuth1ClientSecret,
      kMockTimeSkew, kMockOAuth1Token, kMockOAuth1TokenSecret, kMockScopes,
      kMockOAuth2BaseUrl, kMockOAuth2ClientId, &device_name_service_mock,
      test::kMockSessionId);
  EXPECT_EQ(
      RS_OK,
      request->TryResponse(
          200,
          "{\"access_token\": \"mock-access-token\", "
          "\"refresh_token\": \"mock-refresh-token\", "
          "\"token_type\": \"Bearer\", \"expires_in\": "
          "3601, \"scope\": \"mock-granted-scopes\", \"user_id\": \"1298\"}"));
  EXPECT_NE(nullptr, request->migration_token_response());
  EXPECT_EQ(OAE_OK, request->migration_token_response()->auth_error());
  EXPECT_EQ("mock-refresh-token",
            request->migration_token_response()->refresh_token());
  EXPECT_EQ(ScopeSet::FromEncoded("mock-granted-scopes"),
            request->migration_token_response()->scopes());
  EXPECT_EQ("1298", request->migration_token_response()->user_id());
}

TEST(MigrationTokenRequestTest, TryResponseTokenGrantedNoUserId) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<MigrationTokenRequest> request = MigrationTokenRequest::Create(
      kMockOAuth1BaseUrl, kMockOAuth1ClientId, kMockOAuth1ClientSecret,
      kMockTimeSkew, kMockOAuth1Token, kMockOAuth1TokenSecret, kMockScopes,
      kMockOAuth2BaseUrl, kMockOAuth2ClientId, &device_name_service_mock,
      test::kMockSessionId);
  EXPECT_EQ(RS_PARSE_PROBLEM,
            request->TryResponse(200,
                                 "{\"access_token\": \"mock-access-token\", "
                                 "\"refresh_token\": \"mock-refresh-token\", "
                                 "\"token_type\": \"Bearer\", \"expires_in\": "
                                 "3601, \"scope\": \"mock-granted-scopes\"}"));
  EXPECT_NE(nullptr, request->migration_token_response());
  EXPECT_EQ(OAE_UNSET, request->migration_token_response()->auth_error());
  EXPECT_EQ("", request->migration_token_response()->error_description());
  EXPECT_TRUE(request->migration_token_response()->refresh_token().empty());
  EXPECT_TRUE(request->migration_token_response()->scopes().empty());
  EXPECT_TRUE(request->migration_token_response()->user_id().empty());
}

TEST(MigrationTokenRequestTest, TryResponseWrongTokenType) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<MigrationTokenRequest> request = MigrationTokenRequest::Create(
      kMockOAuth1BaseUrl, kMockOAuth1ClientId, kMockOAuth1ClientSecret,
      kMockTimeSkew, kMockOAuth1Token, kMockOAuth1TokenSecret, kMockScopes,
      kMockOAuth2BaseUrl, kMockOAuth2ClientId, &device_name_service_mock,
      test::kMockSessionId);
  EXPECT_EQ(RS_PARSE_PROBLEM,
            request->TryResponse(200,
                                 "{\"access_token\": \"mock-access-token\", "
                                 "\"refresh_token\": \"mock-refresh-token\", "
                                 "\"token_type\": \"Equador\", \"expires_in\": "
                                 "3601, \"scope\": \"mock-granted-scopes\"}"));
  EXPECT_NE(nullptr, request->migration_token_response());
  EXPECT_EQ(OAE_UNSET, request->migration_token_response()->auth_error());
  EXPECT_EQ("", request->migration_token_response()->error_description());
  EXPECT_EQ("", request->migration_token_response()->refresh_token());
  EXPECT_EQ(ScopeSet(), request->migration_token_response()->scopes());
}

TEST(MigrationTokenRequestTest, TryResponseNegativeExpiresIn) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<MigrationTokenRequest> request = MigrationTokenRequest::Create(
      kMockOAuth1BaseUrl, kMockOAuth1ClientId, kMockOAuth1ClientSecret,
      kMockTimeSkew, kMockOAuth1Token, kMockOAuth1TokenSecret, kMockScopes,
      kMockOAuth2BaseUrl, kMockOAuth2ClientId, &device_name_service_mock,
      test::kMockSessionId);
  EXPECT_EQ(RS_PARSE_PROBLEM,
            request->TryResponse(200,
                                 "{\"access_token\": \"mock-access-token\", "
                                 "\"refresh_token\": \"mock-refresh-token\", "
                                 "\"token_type\": \"Bearer\", \"expires_in\": "
                                 "-911, \"scope\": \"mock-granted-scopes\"}"));
  EXPECT_NE(nullptr, request->migration_token_response());
  EXPECT_EQ(OAE_UNSET, request->migration_token_response()->auth_error());
  EXPECT_EQ("", request->migration_token_response()->error_description());
  EXPECT_EQ("", request->migration_token_response()->refresh_token());
  EXPECT_EQ(ScopeSet(), request->migration_token_response()->scopes());
}

TEST(MigrationTokenRequestTest, TryResponseInvalidRequest) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<MigrationTokenRequest> request = MigrationTokenRequest::Create(
      kMockOAuth1BaseUrl, kMockOAuth1ClientId, kMockOAuth1ClientSecret,
      kMockTimeSkew, kMockOAuth1Token, kMockOAuth1TokenSecret, kMockScopes,
      kMockOAuth2BaseUrl, kMockOAuth2ClientId, &device_name_service_mock,
      test::kMockSessionId);
  EXPECT_EQ(RS_OK,
            request->TryResponse(400, "{\"error\": \"invalid_request\"}"));
  EXPECT_NE(nullptr, request->migration_token_response());
  EXPECT_EQ(OAE_INVALID_REQUEST,
            request->migration_token_response()->auth_error());
  EXPECT_EQ("", request->migration_token_response()->error_description());
  EXPECT_EQ("", request->migration_token_response()->refresh_token());
  EXPECT_EQ(ScopeSet(), request->migration_token_response()->scopes());
}

TEST(MigrationTokenRequestTest, TryResponseInvalidClient) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<MigrationTokenRequest> request = MigrationTokenRequest::Create(
      kMockOAuth1BaseUrl, kMockOAuth1ClientId, kMockOAuth1ClientSecret,
      kMockTimeSkew, kMockOAuth1Token, kMockOAuth1TokenSecret, kMockScopes,
      kMockOAuth2BaseUrl, kMockOAuth2ClientId, &device_name_service_mock,
      test::kMockSessionId);
  EXPECT_EQ(RS_OK,
            request->TryResponse(401, "{\"error\": \"invalid_client\"}"));
  EXPECT_NE(nullptr, request->migration_token_response());
  EXPECT_EQ(OAE_INVALID_CLIENT,
            request->migration_token_response()->auth_error());
  EXPECT_EQ("", request->migration_token_response()->error_description());
  EXPECT_EQ("", request->migration_token_response()->refresh_token());
  EXPECT_EQ(ScopeSet(), request->migration_token_response()->scopes());
}

TEST(MigrationTokenRequestTest, TryResponseInvalidGrant) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<MigrationTokenRequest> request = MigrationTokenRequest::Create(
      kMockOAuth1BaseUrl, kMockOAuth1ClientId, kMockOAuth1ClientSecret,
      kMockTimeSkew, kMockOAuth1Token, kMockOAuth1TokenSecret, kMockScopes,
      kMockOAuth2BaseUrl, kMockOAuth2ClientId, &device_name_service_mock,
      test::kMockSessionId);
  EXPECT_EQ(RS_OK, request->TryResponse(401, "{\"error\": \"invalid_grant\"}"));
  EXPECT_NE(nullptr, request->migration_token_response());
  EXPECT_EQ(OAE_INVALID_GRANT,
            request->migration_token_response()->auth_error());
  EXPECT_EQ("", request->migration_token_response()->error_description());
  EXPECT_EQ("", request->migration_token_response()->refresh_token());
  EXPECT_EQ(ScopeSet(), request->migration_token_response()->scopes());
}

TEST(MigrationTokenRequestTest, TryResponseInvalidScope) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<MigrationTokenRequest> request = MigrationTokenRequest::Create(
      kMockOAuth1BaseUrl, kMockOAuth1ClientId, kMockOAuth1ClientSecret,
      kMockTimeSkew, kMockOAuth1Token, kMockOAuth1TokenSecret, kMockScopes,
      kMockOAuth2BaseUrl, kMockOAuth2ClientId, &device_name_service_mock,
      test::kMockSessionId);
  EXPECT_EQ(RS_OK, request->TryResponse(401, "{\"error\": \"invalid_scope\"}"));
  EXPECT_NE(nullptr, request->migration_token_response());
  EXPECT_EQ(OAE_INVALID_SCOPE,
            request->migration_token_response()->auth_error());
  EXPECT_EQ("", request->migration_token_response()->error_description());
  EXPECT_EQ("", request->migration_token_response()->refresh_token());
  EXPECT_EQ(ScopeSet(), request->migration_token_response()->scopes());
}

TEST(MigrationTokenRequestTest, TryResponseHttpProblem) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<MigrationTokenRequest> request = MigrationTokenRequest::Create(
      kMockOAuth1BaseUrl, kMockOAuth1ClientId, kMockOAuth1ClientSecret,
      kMockTimeSkew, kMockOAuth1Token, kMockOAuth1TokenSecret, kMockScopes,
      kMockOAuth2BaseUrl, kMockOAuth2ClientId, &device_name_service_mock,
      test::kMockSessionId);
  const std::set<int> kValidStatusCodes{200, 400, 401};

  for (int i = 100; i <= 600; i++) {
    if (kValidStatusCodes.find(i) != kValidStatusCodes.end()) {
      EXPECT_EQ(RS_PARSE_PROBLEM, request->TryResponse(i, "garbage OH MY!"))
          << "Wrong response for HTTP status " << i;
    } else {
      EXPECT_EQ(RS_HTTP_PROBLEM, request->TryResponse(i, "garbage OH MY!"))
          << "Wrong response for HTTP status " << i;
    }
    EXPECT_NE(nullptr, request->migration_token_response());
    EXPECT_EQ(OAE_UNSET, request->migration_token_response()->auth_error());
    EXPECT_EQ("", request->migration_token_response()->error_description());
    EXPECT_EQ("", request->migration_token_response()->refresh_token());
    EXPECT_EQ(ScopeSet(), request->migration_token_response()->scopes());
  }
}

TEST(MigrationTokenRequestTest, TryResponseParseProblemHttp200) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<MigrationTokenRequest> request = MigrationTokenRequest::Create(
      kMockOAuth1BaseUrl, kMockOAuth1ClientId, kMockOAuth1ClientSecret,
      kMockTimeSkew, kMockOAuth1Token, kMockOAuth1TokenSecret, kMockScopes,
      kMockOAuth2BaseUrl, kMockOAuth2ClientId, &device_name_service_mock,
      test::kMockSessionId);
  EXPECT_EQ(RS_PARSE_PROBLEM,
            request->TryResponse(200,
                                 "{\"ccess_token\": \"mock-access-token\", "
                                 "\"refresh_token\": \"mock-refresh-token\", "
                                 "\"token_type\": \"Bearer\", \"expires_in\": "
                                 "3601, \"scope\": \"mock-granted-scopes\"}"));
  EXPECT_NE(nullptr, request->migration_token_response());
  EXPECT_EQ(OAE_UNSET, request->migration_token_response()->auth_error());
  EXPECT_EQ("", request->migration_token_response()->error_description());
  EXPECT_EQ("", request->migration_token_response()->refresh_token());
  EXPECT_EQ(ScopeSet(), request->migration_token_response()->scopes());
}

TEST(MigrationTokenRequestTest, TryResponseParseProblemHttp401) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<MigrationTokenRequest> request = MigrationTokenRequest::Create(
      kMockOAuth1BaseUrl, kMockOAuth1ClientId, kMockOAuth1ClientSecret,
      kMockTimeSkew, kMockOAuth1Token, kMockOAuth1TokenSecret, kMockScopes,
      kMockOAuth2BaseUrl, kMockOAuth2ClientId, &device_name_service_mock,
      test::kMockSessionId);
  EXPECT_EQ(RS_PARSE_PROBLEM,
            request->TryResponse(401, "{\"error\": \"grant\"}"));
  EXPECT_NE(nullptr, request->migration_token_response());
  EXPECT_EQ(OAE_UNSET, request->migration_token_response()->auth_error());
  EXPECT_EQ("", request->migration_token_response()->error_description());
  EXPECT_EQ("", request->migration_token_response()->refresh_token());
  EXPECT_EQ(ScopeSet(), request->migration_token_response()->scopes());
}

TEST(MigrationTokenRequestTest, TryResponse400WithErrorDescription) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<MigrationTokenRequest> request = MigrationTokenRequest::Create(
      kMockOAuth1BaseUrl, kMockOAuth1ClientId, kMockOAuth1ClientSecret,
      kMockTimeSkew, kMockOAuth1Token, kMockOAuth1TokenSecret, kMockScopes,
      kMockOAuth2BaseUrl, kMockOAuth2ClientId, &device_name_service_mock,
      test::kMockSessionId);
  EXPECT_EQ(RS_OK, request->TryResponse(400,
                                        "{\"error\": \"invalid_request\", "
                                        "\"error_description\": \"Some error "
                                        "description\"}"));
  EXPECT_NE(nullptr, request->migration_token_response());
  EXPECT_EQ(OAE_INVALID_REQUEST,
            request->migration_token_response()->auth_error());
  EXPECT_EQ("Some error description",
            request->migration_token_response()->error_description());
  EXPECT_EQ("", request->migration_token_response()->refresh_token());
  EXPECT_EQ(ScopeSet(), request->migration_token_response()->scopes());
}

TEST(MigrationTokenRequestTest, TryResponse401WithErrorDescription) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<MigrationTokenRequest> request = MigrationTokenRequest::Create(
      kMockOAuth1BaseUrl, kMockOAuth1ClientId, kMockOAuth1ClientSecret,
      kMockTimeSkew, kMockOAuth1Token, kMockOAuth1TokenSecret, kMockScopes,
      kMockOAuth2BaseUrl, kMockOAuth2ClientId, &device_name_service_mock,
      test::kMockSessionId);
  EXPECT_EQ(RS_OK, request->TryResponse(401,
                                        "{\"error\": \"invalid_scope\", "
                                        "\"error_description\": \"Some error "
                                        "description\"}"));
  EXPECT_NE(nullptr, request->migration_token_response());
  EXPECT_EQ(OAE_INVALID_SCOPE,
            request->migration_token_response()->auth_error());
  EXPECT_EQ("Some error description",
            request->migration_token_response()->error_description());
  EXPECT_EQ("", request->migration_token_response()->refresh_token());
  EXPECT_EQ(ScopeSet(), request->migration_token_response()->scopes());
}

}  // namespace oauth2
}  // namespace opera
