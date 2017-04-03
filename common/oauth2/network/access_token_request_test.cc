// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/network/access_token_request.h"

#include <string>
#include <vector>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "common/oauth2/device_name/device_name_service_mock.h"
#include "common/oauth2/network/access_token_response.h"
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
const std::string kMockDeviceName = "mock_device_name";

}  // namespace

TEST(AccessTokenRequestTest, TokenForAuthTokenRequestContents) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithAuthTokenGrant(
          test::kMockAuthToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(
      "auth_token=mock-auth-token&client_id=mock-client-id&grant_type="
      "auth_token&scope=mock-scope-value-1%20mock-scope-value-2",
      request->GetContent());
  EXPECT_EQ(net::URLFetcher::POST, request->GetHTTPRequestType());
  EXPECT_EQ(kDefaultLoadFlags, request->GetLoadFlags());
  EXPECT_EQ("/oauth2/v1/token/", request->GetPath());
  EXPECT_EQ(std::vector<std::string>{}, request->GetExtraRequestHeaders());
  EXPECT_EQ("sid=mock-session-id", request->GetQueryString());
  EXPECT_EQ(RMUT_OAUTH2, request->GetRequestManagerUrlType());
  EXPECT_EQ(kApplicationXWwwFormUrlencoded, request->GetRequestContentType());
  EXPECT_EQ(test::kMockScopeSet1, request->requested_scopes());
}

TEST(AccessTokenRequestTest,
     TokenForAuthTokenRequestContentsDeviceNameChanged) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(true));
  EXPECT_CALL(device_name_service_mock, GetCurrentDeviceName())
      .WillOnce(Return(kMockDeviceName));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithAuthTokenGrant(
          test::kMockAuthToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(
      "auth_token=mock-auth-"
      "token&client_id=mock-client-id&device_name="
      "mock_device_name&grant_type="
      "auth_token&scope=mock-scope-value-1%20mock-scope-value-2",
      request->GetContent());
  EXPECT_EQ(net::URLFetcher::POST, request->GetHTTPRequestType());
  EXPECT_EQ(kDefaultLoadFlags, request->GetLoadFlags());
  EXPECT_EQ("/oauth2/v1/token/", request->GetPath());
  EXPECT_EQ(std::vector<std::string>{}, request->GetExtraRequestHeaders());
  EXPECT_EQ("sid=mock-session-id", request->GetQueryString());
  EXPECT_EQ(RMUT_OAUTH2, request->GetRequestManagerUrlType());
  EXPECT_EQ(kApplicationXWwwFormUrlencoded, request->GetRequestContentType());
  EXPECT_EQ(test::kMockScopeSet1, request->requested_scopes());
}

TEST(AccessTokenRequestTest,
     TokenForAuthTokenRequestStoresDeviceNameOnSuccess) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(true));
  EXPECT_CALL(device_name_service_mock, GetCurrentDeviceName())
      .WillOnce(Return(kMockDeviceName));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithAuthTokenGrant(
          test::kMockAuthToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_CALL(device_name_service_mock, StoreDeviceName(kMockDeviceName))
      .Times(1);
  EXPECT_EQ(
      RS_OK,
      request->TryResponse(
          200,
          "{\"access_token\": \"mock-access-token\", "
          "\"refresh_token\": \"mock-refresh-token\", "
          "\"token_type\": \"Bearer\", \"expires_in\": "
          "3601, \"scope\": \"mock-granted-scopes\", \"user_id\": \"1298\"}"));
}

TEST(AccessTokenRequestTest,
     TokenForAuthTokenRequestDoesntStoreDeviceNameOnHttpProblem) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(true));
  EXPECT_CALL(device_name_service_mock, GetCurrentDeviceName())
      .WillOnce(Return(kMockDeviceName));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithAuthTokenGrant(
          test::kMockAuthToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_CALL(device_name_service_mock, StoreDeviceName(_)).Times(0);
  EXPECT_EQ(
      RS_HTTP_PROBLEM,
      request->TryResponse(
          1,
          "{\"access_token\": \"mock-access-token\", "
          "\"refresh_token\": \"mock-refresh-token\", "
          "\"token_type\": \"Bearer\", \"expires_in\": "
          "3601, \"scope\": \"mock-granted-scopes\", \"user_id\": \"1298\"}"));
}

TEST(AccessTokenRequestTest,
     TokenForAuthTokenRequestDoesntStoreDeviceNameOnParseProblem) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(true));
  EXPECT_CALL(device_name_service_mock, GetCurrentDeviceName())
      .WillOnce(Return(kMockDeviceName));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithAuthTokenGrant(
          test::kMockAuthToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_CALL(device_name_service_mock, StoreDeviceName(_)).Times(0);
  EXPECT_EQ(
      RS_PARSE_PROBLEM,
      request->TryResponse(
          200,
          "{\"wrong_access_token\": \"mock-access-token\", "
          "\"refresh_token\": \"mock-refresh-token\", "
          "\"token_type\": \"Bearer\", \"expires_in\": "
          "3601, \"scope\": \"mock-granted-scopes\", \"user_id\": \"1298\"}"));
}

TEST(AccessTokenRequestTest,
     TokenForAuthTokenRequestDoesntStoreDeviceNameOnAuthError) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(true));
  EXPECT_CALL(device_name_service_mock, GetCurrentDeviceName())
      .WillOnce(Return(kMockDeviceName));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithAuthTokenGrant(
          test::kMockAuthToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_CALL(device_name_service_mock, StoreDeviceName(_)).Times(0);
  EXPECT_EQ(RS_OK,
            request->TryResponse(401, "{\"error\": \"invalid_client\"}"));
}

TEST(AccessTokenRequestTest, TokenForAuthTokenRequestTryResponseTokenGranted) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithAuthTokenGrant(
          test::kMockAuthToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(
      RS_OK,
      request->TryResponse(
          200,
          "{\"access_token\": \"mock-access-token\", "
          "\"refresh_token\": \"mock-refresh-token\", "
          "\"token_type\": \"Bearer\", \"expires_in\": "
          "3601, \"scope\": \"mock-granted-scopes\", \"user_id\": \"1298\"}"));
  EXPECT_NE(nullptr, request->access_token_response());
  EXPECT_EQ("mock-access-token",
            request->access_token_response()->access_token());
  EXPECT_EQ(OAE_OK, request->access_token_response()->auth_error());
  EXPECT_EQ(base::TimeDelta::FromSeconds(3601),
            request->access_token_response()->expires_in());
  EXPECT_EQ("mock-refresh-token",
            request->access_token_response()->refresh_token());
  EXPECT_EQ(ScopeSet::FromEncoded("mock-granted-scopes"),
            request->access_token_response()->scopes());
  EXPECT_EQ("1298", request->access_token_response()->user_id());
}

TEST(AccessTokenRequestTest,
     TokenForAuthTokenRequestTryResponseTokenGrantedNoUserId) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithAuthTokenGrant(
          test::kMockAuthToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(RS_PARSE_PROBLEM,
            request->TryResponse(200,
                                 "{\"access_token\": \"mock-access-token\", "
                                 "\"refresh_token\": \"mock-refresh-token\", "
                                 "\"token_type\": \"Bearer\", \"expires_in\": "
                                 "3601, \"scope\": \"mock-granted-scopes\"}"));
  EXPECT_NE(nullptr, request->access_token_response());
  EXPECT_TRUE(request->access_token_response()->access_token().empty());
  EXPECT_EQ(OAE_UNSET, request->access_token_response()->auth_error());
  EXPECT_EQ("", request->access_token_response()->error_decription());
  EXPECT_EQ(base::TimeDelta(), request->access_token_response()->expires_in());
  EXPECT_TRUE(request->access_token_response()->refresh_token().empty());
  EXPECT_TRUE(request->access_token_response()->scopes().empty());
  EXPECT_TRUE(request->access_token_response()->user_id().empty());
}

TEST(AccessTokenRequestTest,
     TokenForAuthTokenRequestTryResponseWrongTokenType) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithAuthTokenGrant(
          test::kMockAuthToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(RS_PARSE_PROBLEM,
            request->TryResponse(200,
                                 "{\"access_token\": \"mock-access-token\", "
                                 "\"refresh_token\": \"mock-refresh-token\", "
                                 "\"token_type\": \"Equador\", \"expires_in\": "
                                 "3601, \"scope\": \"mock-granted-scopes\"}"));
  EXPECT_NE(nullptr, request->access_token_response());
  EXPECT_EQ("", request->access_token_response()->access_token());
  EXPECT_EQ(OAE_UNSET, request->access_token_response()->auth_error());
  EXPECT_EQ("", request->access_token_response()->error_decription());
  EXPECT_EQ(base::TimeDelta::FromSeconds(0),
            request->access_token_response()->expires_in());
  EXPECT_EQ("", request->access_token_response()->refresh_token());
  EXPECT_EQ(ScopeSet(), request->access_token_response()->scopes());
}

TEST(AccessTokenRequestTest,
     TokenForAuthTokenRequestTryResponseNegativeExpiresIn) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithAuthTokenGrant(
          test::kMockAuthToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(RS_PARSE_PROBLEM,
            request->TryResponse(200,
                                 "{\"access_token\": \"mock-access-token\", "
                                 "\"refresh_token\": \"mock-refresh-token\", "
                                 "\"token_type\": \"Bearer\", \"expires_in\": "
                                 "-911, \"scope\": \"mock-granted-scopes\"}"));
  EXPECT_NE(nullptr, request->access_token_response());
  EXPECT_EQ("", request->access_token_response()->access_token());
  EXPECT_EQ(OAE_UNSET, request->access_token_response()->auth_error());
  EXPECT_EQ("", request->access_token_response()->error_decription());
  EXPECT_EQ(base::TimeDelta::FromSeconds(0),
            request->access_token_response()->expires_in());
  EXPECT_EQ("", request->access_token_response()->refresh_token());
  EXPECT_EQ(ScopeSet(), request->access_token_response()->scopes());
}

TEST(AccessTokenRequestTest,
     TokenForAuthTokenRequestTryResponseInvalidRequest) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithAuthTokenGrant(
          test::kMockAuthToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(RS_OK,
            request->TryResponse(400, "{\"error\": \"invalid_request\"}"));
  EXPECT_NE(nullptr, request->access_token_response());
  EXPECT_EQ("", request->access_token_response()->access_token());
  EXPECT_EQ(OAE_INVALID_REQUEST,
            request->access_token_response()->auth_error());
  EXPECT_EQ("", request->access_token_response()->error_decription());
  EXPECT_EQ(base::TimeDelta::FromSeconds(0),
            request->access_token_response()->expires_in());
  EXPECT_EQ("", request->access_token_response()->refresh_token());
  EXPECT_EQ(ScopeSet(), request->access_token_response()->scopes());
}

TEST(AccessTokenRequestTest, TokenForAuthTokenRequestTryResponseInvalidClient) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithAuthTokenGrant(
          test::kMockAuthToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(RS_OK,
            request->TryResponse(401, "{\"error\": \"invalid_client\"}"));
  EXPECT_NE(nullptr, request->access_token_response());
  EXPECT_EQ("", request->access_token_response()->access_token());
  EXPECT_EQ(OAE_INVALID_CLIENT, request->access_token_response()->auth_error());
  EXPECT_EQ("", request->access_token_response()->error_decription());
  EXPECT_EQ(base::TimeDelta::FromSeconds(0),
            request->access_token_response()->expires_in());
  EXPECT_EQ("", request->access_token_response()->refresh_token());
  EXPECT_EQ(ScopeSet(), request->access_token_response()->scopes());
}

TEST(AccessTokenRequestTest,
     TokenForAuthTokenRequestTryResponse400WithErrorDescription) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithAuthTokenGrant(
          test::kMockAuthToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(RS_OK, request->TryResponse(400,
                                        "{\"error\": \"invalid_request\", "
                                        "\"error_description\": \"This is some "
                                        "error description\"}"));
  EXPECT_NE(nullptr, request->access_token_response());
  EXPECT_EQ("", request->access_token_response()->access_token());
  EXPECT_EQ(OAE_INVALID_REQUEST,
            request->access_token_response()->auth_error());
  EXPECT_EQ("This is some error description",
            request->access_token_response()->error_decription());
  EXPECT_EQ(base::TimeDelta::FromSeconds(0),
            request->access_token_response()->expires_in());
  EXPECT_EQ("", request->access_token_response()->refresh_token());
  EXPECT_EQ(ScopeSet(), request->access_token_response()->scopes());
}

TEST(AccessTokenRequestTest,
     TokenForAuthTokenRequestTryResponse401WithErrorDescription) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithAuthTokenGrant(
          test::kMockAuthToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(RS_OK, request->TryResponse(401,
                                        "{\"error\": \"invalid_client\", "
                                        "\"error_description\": \"This is some "
                                        "error description\"}"));
  EXPECT_NE(nullptr, request->access_token_response());
  EXPECT_EQ("", request->access_token_response()->access_token());
  EXPECT_EQ(OAE_INVALID_CLIENT, request->access_token_response()->auth_error());
  EXPECT_EQ("This is some error description",
            request->access_token_response()->error_decription());
  EXPECT_EQ(base::TimeDelta::FromSeconds(0),
            request->access_token_response()->expires_in());
  EXPECT_EQ("", request->access_token_response()->refresh_token());
  EXPECT_EQ(ScopeSet(), request->access_token_response()->scopes());
}

TEST(AccessTokenRequestTest, TokenForAuthTokenRequestTryResponseInvalidGrant) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithAuthTokenGrant(
          test::kMockAuthToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(RS_OK, request->TryResponse(401, "{\"error\": \"invalid_grant\"}"));
  EXPECT_NE(nullptr, request->access_token_response());
  EXPECT_EQ("", request->access_token_response()->access_token());
  EXPECT_EQ(OAE_INVALID_GRANT, request->access_token_response()->auth_error());
  EXPECT_EQ("", request->access_token_response()->error_decription());
  EXPECT_EQ(base::TimeDelta::FromSeconds(0),
            request->access_token_response()->expires_in());
  EXPECT_EQ("", request->access_token_response()->refresh_token());
  EXPECT_EQ(ScopeSet(), request->access_token_response()->scopes());
}

TEST(AccessTokenRequestTest, TokenForAuthTokenRequestTryResponseInvalidScope) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithAuthTokenGrant(
          test::kMockAuthToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(RS_OK, request->TryResponse(401, "{\"error\": \"invalid_scope\"}"));
  EXPECT_NE(nullptr, request->access_token_response());
  EXPECT_EQ("", request->access_token_response()->access_token());
  EXPECT_EQ(OAE_INVALID_SCOPE, request->access_token_response()->auth_error());
  EXPECT_EQ("", request->access_token_response()->error_decription());
  EXPECT_EQ(base::TimeDelta::FromSeconds(0),
            request->access_token_response()->expires_in());
  EXPECT_EQ("", request->access_token_response()->refresh_token());
  EXPECT_EQ(ScopeSet(), request->access_token_response()->scopes());
}

TEST(AccessTokenRequestTest, TokenForAuthTokenRequestTryResponseHttpProblem) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithAuthTokenGrant(
          test::kMockAuthToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  const std::set<int> kValidStatusCodes{200, 400, 401};

  for (int i = 100; i <= 600; i++) {
    if (kValidStatusCodes.find(i) != kValidStatusCodes.end()) {
      EXPECT_EQ(RS_PARSE_PROBLEM, request->TryResponse(i, "garbage OH MY!"))
          << "Wrong response for HTTP status " << i;
    } else {
      EXPECT_EQ(RS_HTTP_PROBLEM, request->TryResponse(i, "garbage OH MY!"))
          << "Wrong response for HTTP status " << i;
    }
    EXPECT_NE(nullptr, request->access_token_response());
    EXPECT_EQ("", request->access_token_response()->access_token());
    EXPECT_EQ(OAE_UNSET, request->access_token_response()->auth_error());
    EXPECT_EQ("", request->access_token_response()->error_decription());
    EXPECT_EQ(base::TimeDelta::FromSeconds(0),
              request->access_token_response()->expires_in());
    EXPECT_EQ("", request->access_token_response()->refresh_token());
    EXPECT_EQ(ScopeSet(), request->access_token_response()->scopes());
  }
}

TEST(AccessTokenRequestTest,
     TokenForAuthTokenRequestTryResponseParseProblemHttp200) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithAuthTokenGrant(
          test::kMockAuthToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(RS_PARSE_PROBLEM,
            request->TryResponse(200,
                                 "{\"ccess_token\": \"mock-access-token\", "
                                 "\"refresh_token\": \"mock-refresh-token\", "
                                 "\"token_type\": \"Bearer\", \"expires_in\": "
                                 "3601, \"scope\": \"mock-granted-scopes\"}"));
  EXPECT_NE(nullptr, request->access_token_response());
  EXPECT_EQ("", request->access_token_response()->access_token());
  EXPECT_EQ(OAE_UNSET, request->access_token_response()->auth_error());
  EXPECT_EQ("", request->access_token_response()->error_decription());
  EXPECT_EQ(base::TimeDelta::FromSeconds(0),
            request->access_token_response()->expires_in());
  EXPECT_EQ("", request->access_token_response()->refresh_token());
  EXPECT_EQ(ScopeSet(), request->access_token_response()->scopes());
}

TEST(AccessTokenRequestTest,
     TokenForAuthTokenRequestTryResponseParseProblemHttp401) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithAuthTokenGrant(
          test::kMockAuthToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(RS_PARSE_PROBLEM,
            request->TryResponse(401, "{\"error\": \"grant\"}"));
  EXPECT_NE(nullptr, request->access_token_response());
  EXPECT_EQ("", request->access_token_response()->access_token());
  EXPECT_EQ(OAE_UNSET, request->access_token_response()->auth_error());
  EXPECT_EQ("", request->access_token_response()->error_decription());
  EXPECT_EQ(base::TimeDelta::FromSeconds(0),
            request->access_token_response()->expires_in());
  EXPECT_EQ("", request->access_token_response()->refresh_token());
  EXPECT_EQ(ScopeSet(), request->access_token_response()->scopes());
}

TEST(AccessTokenRequestTest, TokenForRefreshTokenRequestContents) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithRefreshTokenGrant(
          test::kMockRefreshToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(request->GetContent(),
            "client_id=mock-client-id&grant_"
            "type=refresh_token&refresh_token=mock-refresh-token&scope=mock-"
            "scope-value-1%20mock-scope-value-2");
  EXPECT_EQ(net::URLFetcher::POST, request->GetHTTPRequestType());
  EXPECT_EQ(kDefaultLoadFlags, request->GetLoadFlags());
  EXPECT_EQ("/oauth2/v1/token/", request->GetPath());
  EXPECT_EQ(std::vector<std::string>{}, request->GetExtraRequestHeaders());
  EXPECT_EQ("sid=mock-session-id", request->GetQueryString());
  EXPECT_EQ(RMUT_OAUTH2, request->GetRequestManagerUrlType());
  EXPECT_EQ(kApplicationXWwwFormUrlencoded, request->GetRequestContentType());
  EXPECT_EQ(test::kMockScopeSet1, request->requested_scopes());
}

TEST(AccessTokenRequestTest,
     TokenForRefreshTokenRequestContentsDeviceNameChanged) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(true));
  EXPECT_CALL(device_name_service_mock, GetCurrentDeviceName())
      .WillOnce(Return(kMockDeviceName));

  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithRefreshTokenGrant(
          test::kMockRefreshToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(request->GetContent(),
            "client_id=mock-client-id&device_"
            "name=mock_device_name&grant_"
            "type=refresh_token&refresh_token=mock-refresh-token&scope=mock-"
            "scope-value-1%20mock-scope-value-2");
  EXPECT_EQ(net::URLFetcher::POST, request->GetHTTPRequestType());
  EXPECT_EQ(kDefaultLoadFlags, request->GetLoadFlags());
  EXPECT_EQ("/oauth2/v1/token/", request->GetPath());
  EXPECT_EQ(std::vector<std::string>{}, request->GetExtraRequestHeaders());
  EXPECT_EQ("sid=mock-session-id", request->GetQueryString());
  EXPECT_EQ(RMUT_OAUTH2, request->GetRequestManagerUrlType());
  EXPECT_EQ(kApplicationXWwwFormUrlencoded, request->GetRequestContentType());
  EXPECT_EQ(test::kMockScopeSet1, request->requested_scopes());
}

TEST(AccessTokenRequestTest,
     TokenForRefreshTokenRequestStoresDeviceNameOnSuccess) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(true));
  EXPECT_CALL(device_name_service_mock, GetCurrentDeviceName())
      .WillOnce(Return(kMockDeviceName));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithRefreshTokenGrant(
          test::kMockRefreshToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_CALL(device_name_service_mock, StoreDeviceName(kMockDeviceName))
      .Times(1);
  EXPECT_EQ(
      RS_OK,
      request->TryResponse(
          200,
          "{\"access_token\": \"mock-access-token\", "
          "\"refresh_token\": \"mock-refresh-token\", "
          "\"token_type\": \"Bearer\", \"expires_in\": "
          "3601, \"scope\": \"mock-granted-scopes\", \"user_id\": \"1298\"}"));
}

TEST(AccessTokenRequestTest,
     TokenForRefreshTokenRequestDoesntStoreDeviceNameOnHttpProblem) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(true));
  EXPECT_CALL(device_name_service_mock, GetCurrentDeviceName())
      .WillOnce(Return(kMockDeviceName));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithRefreshTokenGrant(
          test::kMockRefreshToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_CALL(device_name_service_mock, StoreDeviceName(_)).Times(0);
  EXPECT_EQ(
      RS_HTTP_PROBLEM,
      request->TryResponse(
          1,
          "{\"access_token\": \"mock-access-token\", "
          "\"refresh_token\": \"mock-refresh-token\", "
          "\"token_type\": \"Bearer\", \"expires_in\": "
          "3601, \"scope\": \"mock-granted-scopes\", \"user_id\": \"1298\"}"));
}

TEST(AccessTokenRequestTest,
     TokenForRefreshTokenRequestDoesntStoreDeviceNameOnParseProblem) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(true));
  EXPECT_CALL(device_name_service_mock, GetCurrentDeviceName())
      .WillOnce(Return(kMockDeviceName));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithRefreshTokenGrant(
          test::kMockRefreshToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_CALL(device_name_service_mock, StoreDeviceName(_)).Times(0);
  EXPECT_EQ(
      RS_PARSE_PROBLEM,
      request->TryResponse(
          200,
          "{\"wrong_access_token\": \"mock-access-token\", "
          "\"refresh_token\": \"mock-refresh-token\", "
          "\"token_type\": \"Bearer\", \"expires_in\": "
          "3601, \"scope\": \"mock-granted-scopes\", \"user_id\": \"1298\"}"));
}

TEST(AccessTokenRequestTest,
     TokenForRefreshTokenRequestDoesntStoreDeviceNameOnAuthError) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(true));
  EXPECT_CALL(device_name_service_mock, GetCurrentDeviceName())
      .WillOnce(Return(kMockDeviceName));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithRefreshTokenGrant(
          test::kMockRefreshToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_CALL(device_name_service_mock, StoreDeviceName(_)).Times(0);
  EXPECT_EQ(RS_OK,
            request->TryResponse(401, "{\"error\": \"invalid_client\"}"));
}

TEST(AccessTokenRequestTest, TokenForRefreshTokenTryResponseTokenGranted) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithRefreshTokenGrant(
          test::kMockRefreshToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(RS_OK,
            request->TryResponse(200,
                                 "{\"access_token\": \"mock-access-token\", "
                                 "\"token_type\": \"Bearer\", \"expires_in\": "
                                 "3601, \"scope\": \"mock-granted-scopes\"}"));
  EXPECT_NE(request->access_token_response(), nullptr);
  EXPECT_EQ(request->access_token_response()->access_token(),
            "mock-access-token");
  EXPECT_EQ(OAE_OK, request->access_token_response()->auth_error());
  EXPECT_EQ("", request->access_token_response()->error_decription());
  EXPECT_EQ(base::TimeDelta::FromSeconds(3601),
            request->access_token_response()->expires_in());
  EXPECT_EQ(ScopeSet::FromEncoded("mock-granted-scopes"),
            request->access_token_response()->scopes());
  EXPECT_TRUE(request->access_token_response()->refresh_token().empty());
  EXPECT_TRUE(request->access_token_response()->user_id().empty());
}

TEST(AccessTokenRequestTest,
     TokenForRefreshTokenTryResponseHasRefreshTokenNoUserId) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithRefreshTokenGrant(
          test::kMockRefreshToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(RS_OK,
            request->TryResponse(200,
                                 "{\"access_token\": \"mock-access-token\", "
                                 "\"token_type\": \"Bearer\", \"expires_in\": "
                                 "3601, \"scope\": \"mock-granted-scopes\", "
                                 "\"refresh_token\": \"mock-refresh-token\"}"));
  EXPECT_NE(request->access_token_response(), nullptr);
  EXPECT_EQ("mock-access-token",
            request->access_token_response()->access_token());
  EXPECT_EQ(OAE_OK, request->access_token_response()->auth_error());
  EXPECT_EQ("", request->access_token_response()->error_decription());
  EXPECT_EQ(base::TimeDelta::FromSeconds(3601),
            request->access_token_response()->expires_in());
  EXPECT_EQ("mock-granted-scopes",
            request->access_token_response()->scopes().encode());
  EXPECT_EQ("mock-refresh-token",
            request->access_token_response()->refresh_token());
  EXPECT_TRUE(request->access_token_response()->user_id().empty());
}

TEST(AccessTokenRequestTest,
     TokenForRefreshTokenTryResponseHasRefreshTokenAndUserId) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithRefreshTokenGrant(
          test::kMockRefreshToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(RS_OK,
            request->TryResponse(200,
                                 "{\"access_token\": \"mock-access-token\", "
                                 "\"token_type\": \"Bearer\", \"expires_in\": "
                                 "3601, \"scope\": \"mock-granted-scopes\", "
                                 "\"refresh_token\": \"mock-refresh-token\", "
                                 "\"user_id\": \"1122\"}"));
  EXPECT_NE(request->access_token_response(), nullptr);
  EXPECT_EQ("mock-access-token",
            request->access_token_response()->access_token());
  EXPECT_EQ(OAE_OK, request->access_token_response()->auth_error());
  EXPECT_EQ("", request->access_token_response()->error_decription());
  EXPECT_EQ(base::TimeDelta::FromSeconds(3601),
            request->access_token_response()->expires_in());
  EXPECT_EQ(ScopeSet::FromEncoded("mock-granted-scopes"),
            request->access_token_response()->scopes());
  EXPECT_EQ("mock-refresh-token",
            request->access_token_response()->refresh_token());
  EXPECT_EQ("1122", request->access_token_response()->user_id());
}

TEST(AccessTokenRequestTest, TokenForRefreshTokenTryResponseWrongTokenType) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithRefreshTokenGrant(
          test::kMockRefreshToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(RS_PARSE_PROBLEM,
            request->TryResponse(200,
                                 "{\"access_token\": \"mock-access-token\", "
                                 "\"token_type\": \"Equador\", \"expires_in\": "
                                 "3601, \"scope\": \"mock-granted-scopes\"}"));
  EXPECT_NE(nullptr, request->access_token_response());
  EXPECT_EQ("", request->access_token_response()->access_token());
  EXPECT_EQ(OAE_UNSET, request->access_token_response()->auth_error());
  EXPECT_EQ("", request->access_token_response()->error_decription());
  EXPECT_EQ(base::TimeDelta::FromSeconds(0),
            request->access_token_response()->expires_in());
  EXPECT_EQ(ScopeSet(), request->access_token_response()->scopes());
  EXPECT_TRUE(request->access_token_response()->refresh_token().empty());
  EXPECT_TRUE(request->access_token_response()->user_id().empty());
}

TEST(AccessTokenRequestTest, TokenForRefreshTokenTryResponseNegativeExpiresIn) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithRefreshTokenGrant(
          test::kMockRefreshToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(RS_PARSE_PROBLEM,
            request->TryResponse(200,
                                 "{\"access_token\": \"mock-access-token\", "
                                 "\"token_type\": \"Bearer\", \"expires_in\": "
                                 "-944, \"scope\": \"mock-granted-scopes\"}"));
  EXPECT_NE(nullptr, request->access_token_response());
  EXPECT_EQ("", request->access_token_response()->access_token());
  EXPECT_EQ(OAE_UNSET, request->access_token_response()->auth_error());
  EXPECT_EQ("", request->access_token_response()->error_decription());
  EXPECT_EQ(base::TimeDelta::FromSeconds(0),
            request->access_token_response()->expires_in());
  EXPECT_EQ(ScopeSet(), request->access_token_response()->scopes());
  EXPECT_TRUE(request->access_token_response()->refresh_token().empty());
  EXPECT_TRUE(request->access_token_response()->user_id().empty());
}

TEST(AccessTokenRequestTest, TokenForRefreshTokenTryResponseInvalidRequest) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithRefreshTokenGrant(
          test::kMockRefreshToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(RS_OK,
            request->TryResponse(400, "{\"error\": \"invalid_request\"}"));
  EXPECT_NE(nullptr, request->access_token_response());
  EXPECT_EQ("", request->access_token_response()->access_token());
  EXPECT_EQ(OAE_INVALID_REQUEST,
            request->access_token_response()->auth_error());
  EXPECT_EQ("", request->access_token_response()->error_decription());
  EXPECT_EQ(base::TimeDelta::FromSeconds(0),
            request->access_token_response()->expires_in());
  EXPECT_EQ(ScopeSet(), request->access_token_response()->scopes());
  EXPECT_TRUE(request->access_token_response()->refresh_token().empty());
  EXPECT_TRUE(request->access_token_response()->user_id().empty());
}

TEST(AccessTokenRequestTest, TokenForRefreshTokenTryResponseInvalidClient) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithRefreshTokenGrant(
          test::kMockRefreshToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(RS_OK,
            request->TryResponse(401, "{\"error\": \"invalid_client\"}"));
  EXPECT_NE(nullptr, request->access_token_response());
  EXPECT_EQ("", request->access_token_response()->access_token());
  EXPECT_EQ(OAE_INVALID_CLIENT, request->access_token_response()->auth_error());
  EXPECT_EQ("", request->access_token_response()->error_decription());
  EXPECT_EQ(base::TimeDelta::FromSeconds(0),
            request->access_token_response()->expires_in());
  EXPECT_EQ(ScopeSet(), request->access_token_response()->scopes());
  EXPECT_TRUE(request->access_token_response()->refresh_token().empty());
  EXPECT_TRUE(request->access_token_response()->user_id().empty());
}

TEST(AccessTokenRequestTest, TokenForRefreshTokenTryResponseInvalidGrant) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithRefreshTokenGrant(
          test::kMockRefreshToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(RS_OK, request->TryResponse(401, "{\"error\": \"invalid_grant\"}"));
  EXPECT_NE(nullptr, request->access_token_response());
  EXPECT_EQ("", request->access_token_response()->access_token());
  EXPECT_EQ(OAE_INVALID_GRANT, request->access_token_response()->auth_error());
  EXPECT_EQ("", request->access_token_response()->error_decription());
  EXPECT_EQ(base::TimeDelta::FromSeconds(0),
            request->access_token_response()->expires_in());
  EXPECT_EQ(ScopeSet(), request->access_token_response()->scopes());
  EXPECT_TRUE(request->access_token_response()->refresh_token().empty());
  EXPECT_TRUE(request->access_token_response()->user_id().empty());
}

TEST(AccessTokenRequestTest, TokenForRefreshTokenTryResponseInvalidScope) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithRefreshTokenGrant(
          test::kMockRefreshToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(RS_OK, request->TryResponse(401, "{\"error\": \"invalid_scope\"}"));
  EXPECT_NE(nullptr, request->access_token_response());
  EXPECT_EQ("", request->access_token_response()->access_token());
  EXPECT_EQ(OAE_INVALID_SCOPE, request->access_token_response()->auth_error());
  EXPECT_EQ("", request->access_token_response()->error_decription());
  EXPECT_EQ(base::TimeDelta::FromSeconds(0),
            request->access_token_response()->expires_in());
  EXPECT_EQ(ScopeSet(), request->access_token_response()->scopes());
  EXPECT_TRUE(request->access_token_response()->refresh_token().empty());
  EXPECT_TRUE(request->access_token_response()->user_id().empty());
}

TEST(AccessTokenRequestTest, TokenForRefreshTokenTryResponseHttpProblem) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithRefreshTokenGrant(
          test::kMockRefreshToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  const std::set<int> kValidStatusCodes{200, 400, 401};

  for (int i = 100; i <= 600; i++) {
    if (kValidStatusCodes.find(i) != kValidStatusCodes.end()) {
      EXPECT_EQ(RS_PARSE_PROBLEM, request->TryResponse(i, "garbage OH MY!"))
          << "Wrong response for HTTP status " << i;
    } else {
      EXPECT_EQ(RS_HTTP_PROBLEM, request->TryResponse(i, "garbage OH MY!"))
          << "Wrong response for HTTP status " << i;
    }
    EXPECT_NE(nullptr, request->access_token_response());
    EXPECT_EQ("", request->access_token_response()->access_token());
    EXPECT_EQ(OAE_UNSET, request->access_token_response()->auth_error());
    EXPECT_EQ("", request->access_token_response()->error_decription());
    EXPECT_EQ(base::TimeDelta::FromSeconds(0),
              request->access_token_response()->expires_in());
    EXPECT_EQ(ScopeSet(), request->access_token_response()->scopes());
    EXPECT_TRUE(request->access_token_response()->refresh_token().empty());
    EXPECT_TRUE(request->access_token_response()->user_id().empty());
  }
}

TEST(AccessTokenRequestTest,
     TokenForRefreshTokenTryResponseParseProblemHttp200) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithRefreshTokenGrant(
          test::kMockRefreshToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(RS_PARSE_PROBLEM,
            request->TryResponse(200,
                                 "{\"ccess_token\": \"mock-access-token\", "
                                 "\"refresh_token\": \"mock-refresh-token\", "
                                 "\"token_type\": \"Bearer\", \"expires_in\": "
                                 "3601, \"scope\": \"mock-granted-scopes\"}"));
  EXPECT_NE(nullptr, request->access_token_response());
  EXPECT_EQ("", request->access_token_response()->access_token());
  EXPECT_EQ(OAE_UNSET, request->access_token_response()->auth_error());
  EXPECT_EQ("", request->access_token_response()->error_decription());
  EXPECT_EQ(base::TimeDelta::FromSeconds(0),
            request->access_token_response()->expires_in());
  EXPECT_EQ(ScopeSet(), request->access_token_response()->scopes());
  EXPECT_TRUE(request->access_token_response()->refresh_token().empty());
  EXPECT_TRUE(request->access_token_response()->user_id().empty());
}

TEST(AccessTokenRequestTest,
     TokenForRefreshTokenTryResponseParseProblemHttp401) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithRefreshTokenGrant(
          test::kMockRefreshToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(RS_PARSE_PROBLEM,
            request->TryResponse(401, "{\"error\": \"grant\"}"));
  EXPECT_NE(nullptr, request->access_token_response());
  EXPECT_EQ("", request->access_token_response()->access_token());
  EXPECT_EQ(OAE_UNSET, request->access_token_response()->auth_error());
  EXPECT_EQ("", request->access_token_response()->error_decription());
  EXPECT_EQ(base::TimeDelta::FromSeconds(0),
            request->access_token_response()->expires_in());
  EXPECT_EQ(ScopeSet(), request->access_token_response()->scopes());
  EXPECT_TRUE(request->access_token_response()->refresh_token().empty());
  EXPECT_TRUE(request->access_token_response()->user_id().empty());
}

TEST(AccessTokenRequestTest,
     TokenForRefreshTokenTryResponse400WithErrorDescription) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithRefreshTokenGrant(
          test::kMockRefreshToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(RS_OK, request->TryResponse(400,
                                        "{\"error\": \"invalid_request\", "
                                        "\"error_description\": \"This is some "
                                        "error description\"}"));
  EXPECT_NE(nullptr, request->access_token_response());
  EXPECT_EQ("", request->access_token_response()->access_token());
  EXPECT_EQ(OAE_INVALID_REQUEST,
            request->access_token_response()->auth_error());
  EXPECT_EQ("This is some error description",
            request->access_token_response()->error_decription());
  EXPECT_EQ(base::TimeDelta::FromSeconds(0),
            request->access_token_response()->expires_in());
  EXPECT_EQ(ScopeSet(), request->access_token_response()->scopes());
  EXPECT_TRUE(request->access_token_response()->refresh_token().empty());
  EXPECT_TRUE(request->access_token_response()->user_id().empty());
}

TEST(AccessTokenRequestTest,
     TokenForRefreshTokenTryResponse401WithErrorDescription) {
  DeviceNameServiceMock device_name_service_mock;
  EXPECT_CALL(device_name_service_mock, HasDeviceNameChanged())
      .WillOnce(Return(false));
  scoped_refptr<AccessTokenRequest> request =
      AccessTokenRequest::CreateWithRefreshTokenGrant(
          test::kMockRefreshToken, test::kMockScopeSet1, test::kMockClientId,
          &device_name_service_mock, test::kMockSessionId);
  EXPECT_EQ(RS_OK, request->TryResponse(401,
                                        "{\"error\": \"invalid_scope\", "
                                        "\"error_description\": \"This is some "
                                        "error description\"}"));
  EXPECT_NE(nullptr, request->access_token_response());
  EXPECT_EQ("", request->access_token_response()->access_token());
  EXPECT_EQ(OAE_INVALID_SCOPE, request->access_token_response()->auth_error());
  EXPECT_EQ("This is some error description",
            request->access_token_response()->error_decription());
  EXPECT_EQ(base::TimeDelta::FromSeconds(0),
            request->access_token_response()->expires_in());
  EXPECT_EQ(ScopeSet(), request->access_token_response()->scopes());
  EXPECT_TRUE(request->access_token_response()->refresh_token().empty());
  EXPECT_TRUE(request->access_token_response()->user_id().empty());
}

}  // namespace oauth2
}  // namespace opera
