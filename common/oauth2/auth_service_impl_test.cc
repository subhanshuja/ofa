// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/auth_service_impl.h"

#include <memory>
#include <string>

#include "base/files/scoped_temp_dir.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/threading/thread.h"
#include "components/prefs/testing_pref_service.h"
#include "net/http/http_status_code.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "common/oauth2/client/auth_service_client.h"
#include "common/oauth2/device_name/device_name_service_mock.h"
#include "common/oauth2/migration/oauth1_migrator.h"
#include "common/oauth2/network/access_token_request.h"
#include "common/oauth2/network/network_request_mock.h"
#include "common/oauth2/network/network_request_manager_mock.h"
#include "common/oauth2/session/session_state_observer.h"
#include "common/oauth2/session/persistent_session_mock.h"
#include "common/oauth2/network/revoke_token_request.h"
#include "common/oauth2/test/testing_constants.h"
#include "common/oauth2/token_cache/token_cache_mock.h"
#include "common/oauth2/token_cache/token_cache_table.h"
#include "common/oauth2/token_cache/token_cache_webdata.h"
#include "common/oauth2/util/init_params.h"

namespace opera {
namespace oauth2 {
namespace {

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::InSequence;
using ::testing::InvokeWithoutArgs;
using ::testing::Return;

class MockSessionStateObserver : public SessionStateObserver {
 public:
  MOCK_METHOD0(OnSessionStateChanged, void());
};

class MockOAuth1Migrator : public OAuth1Migrator {
 public:
  MOCK_METHOD0(IsMigrationPossible, bool());
  MOCK_METHOD1(PrepareMigration, void(PersistentSession*));
  MOCK_METHOD0(StartMigration, void());
  MOCK_METHOD0(EnsureOAuth1SessionIsCleared, void());
  MOCK_METHOD0(GetMigrationResult, MigrationResult());
};

class MockAuthServiceClient : public AuthServiceClient {
 public:
  MOCK_METHOD2(OnAccessTokenRequestCompleted,
               void(AuthService* service, RequestAccessTokenResponse response));
  MOCK_METHOD2(OnAccessTokenRequestDenied,
               void(AuthService* service, ScopeSet requested_scopes));
};

}  // namespace

class AuthServiceImplTest : public ::testing::Test {
 public:
  AuthServiceImplTest() {}
  ~AuthServiceImplTest() override {}

  void SetUp() override {
    service_.reset(new AuthServiceImpl());
    init_params_.reset(new InitParams);
    token_cache_mock_.reset(new TokenCacheMock);
    session_mock_ = new PersistentSessionMock;
    request_manager_mock_ = new NetworkRequestManagerMock;
    mock_oauth1_migrator_ = new MockOAuth1Migrator;

    init_params_->client_id = "fake-client-id-string";
    init_params_->pref_service = &prefs_;
    init_params_->oauth2_token_cache = token_cache_mock_.get();
    init_params_->oauth2_session =
        base::WrapUnique<PersistentSession>(session_mock_);
    init_params_->oauth2_network_request_manager =
        base::WrapUnique<NetworkRequestManager>(request_manager_mock_);
    init_params_->oauth1_migrator = base::WrapUnique(mock_oauth1_migrator_);
    init_params_->device_name_service = &device_name_service_mock_;
    test_task_runner_ = new base::TestMockTimeTaskRunner;
    init_params_->task_runner = test_task_runner_;

    service_->AddSessionStateObserver(&mock_session_state_observer_);
  }

 protected:
  scoped_refptr<base::TestMockTimeTaskRunner> test_task_runner_;
  std::unique_ptr<AuthServiceImpl> service_;
  std::unique_ptr<InitParams> init_params_;
  TestingPrefServiceSimple prefs_;
  std::unique_ptr<TokenCacheMock> token_cache_mock_;
  PersistentSessionMock* session_mock_;
  NetworkRequestManagerMock* request_manager_mock_;
  MockOAuth1Migrator* mock_oauth1_migrator_;
  MockSessionStateObserver mock_session_state_observer_;
  DeviceNameServiceMock device_name_service_mock_;
};

TEST_F(AuthServiceImplTest, DiagnosticSnapshotContents) {
  EXPECT_CALL(*token_cache_mock_, Load(_)).Times(1);
  EXPECT_CALL(*session_mock_, Initialize(_)).Times(1);
  EXPECT_CALL(*session_mock_, LoadSession()).Times(1);
  EXPECT_CALL(*session_mock_, GetState())
      .WillRepeatedly(Return(OA2SS_INACTIVE));
  EXPECT_CALL(*mock_oauth1_migrator_, IsMigrationPossible())
      .WillOnce(Return(false));

  service_->Initialize(std::move(init_params_));
  std::unique_ptr<base::DictionaryValue> detail(new base::DictionaryValue);

  detail->SetString("last_session_end_reason", "UNKNOWN");
  detail->Set("clients", new base::ListValue);
  detail->SetString("last_session_end_reason", "UNKNOWN");
  detail->SetInteger("pending_access_token_requests", 0);
  detail->SetBoolean("refresh_token_request_pending", false);

  EXPECT_EQ("auth_service", service_->GetDiagnosticName());
  const auto& actual = service_->GetDiagnosticSnapshot();
  EXPECT_NE(nullptr, actual);
  EXPECT_TRUE(detail->Equals(actual.get())) << "Actual: " << *actual
                                            << "Expected: " << *detail;
}

TEST_F(AuthServiceImplTest, StartsSessionNoMigration) {
  // Starting the session succeeds.
  EXPECT_CALL(*token_cache_mock_, Load(_));
  EXPECT_CALL(*session_mock_, Initialize(_));
  EXPECT_CALL(*session_mock_, LoadSession());
  EXPECT_CALL(*session_mock_, GetState())
      .WillRepeatedly(Return(OA2SS_INACTIVE));
  EXPECT_CALL(*mock_oauth1_migrator_, IsMigrationPossible())
      .WillOnce(Return(false));
  EXPECT_CALL(*session_mock_, SetState(OA2SS_INACTIVE));
  EXPECT_CALL(*session_mock_, SetState(OA2SS_STARTING));
  EXPECT_CALL(*session_mock_, GetSessionId())
      .WillRepeatedly(Return(test::kMockSessionId));
  EXPECT_CALL(*session_mock_, SetState(OA2SS_IN_PROGRESS));
  EXPECT_CALL(*session_mock_, SetStartMethod(SSM_AUTH_TOKEN));

  EXPECT_CALL(*session_mock_, SetUsername("mock-username"));
  EXPECT_CALL(*session_mock_, SetUserId("12348"));
  EXPECT_CALL(*session_mock_, SetRefreshToken("mock-refresh-token"));
  EXPECT_CALL(*session_mock_, StoreSession());

  EXPECT_CALL(device_name_service_mock_, HasDeviceNameChanged())
      .WillOnce(Return(false));

  scoped_refptr<NetworkRequest> access_token_request;
  EXPECT_CALL(*request_manager_mock_, StartRequest(_, _))
      .WillOnce(::testing::SaveArg<0>(&access_token_request));

  service_->Initialize(std::move(init_params_));
  service_->OnTokenCacheLoaded();

  service_->StartSessionWithAuthToken(test::kMockUsername,
                                      test::kMockAuthToken);

  EXPECT_NE(nullptr, access_token_request.get());
  AccessTokenRequest* atr =
      static_cast<AccessTokenRequest*>(access_token_request.get());

  EXPECT_EQ(
      RS_OK,
      atr->TryResponse(
          net::HTTP_OK,
          "{\"access_token\": \"mock-access-token\", "
          "\"refresh_token\": \"mock-refresh-token\", "
          "\"token_type\": \"Bearer\", \"expires_in\": "
          "3601, \"scope\": \"mock-granted-scopes\", \"user_id\": \"12348\"}"));
  EXPECT_CALL(*session_mock_, GetStartMethod())
      .WillOnce(Return(SSM_AUTH_TOKEN));
  service_->OnNetworkRequestFinished(access_token_request, RS_OK);
}

TEST_F(AuthServiceImplTest, StartsSessionWithMigration) {
  EXPECT_CALL(*token_cache_mock_, Load(_));
  EXPECT_CALL(*session_mock_, Initialize(_));
  EXPECT_CALL(*session_mock_, LoadSession());
  EXPECT_CALL(*session_mock_, GetState())
      .WillOnce(Return(OA2SS_INACTIVE))
      .WillOnce(Return(OA2SS_STARTING));
  EXPECT_CALL(*mock_oauth1_migrator_, IsMigrationPossible())
      .WillRepeatedly(Return(true));

  EXPECT_CALL(*mock_oauth1_migrator_, PrepareMigration(session_mock_));
  EXPECT_CALL(*mock_oauth1_migrator_, StartMigration());
  service_->Initialize(std::move(init_params_));
  service_->OnTokenCacheLoaded();

  // OAuth1 migrator will continue with starting the session, scenarios
  // tested in migrator tests.
}

TEST_F(AuthServiceImplTest, EndSessionCleansUp) {
  EXPECT_CALL(*token_cache_mock_, Load(_));
  EXPECT_CALL(*session_mock_, Initialize(_));
  EXPECT_CALL(*session_mock_, LoadSession());
  EXPECT_CALL(*session_mock_, GetState())
      .WillRepeatedly(Return(OA2SS_IN_PROGRESS));

  service_->Initialize(std::move(init_params_));
  service_->OnTokenCacheLoaded();

  EXPECT_CALL(*session_mock_, SetState(OA2SS_FINISHING));
  EXPECT_CALL(*session_mock_, ClearSession());
  EXPECT_CALL(*session_mock_, StoreSession());
  EXPECT_CALL(*session_mock_, GetRefreshToken())
      .WillRepeatedly(Return("mock-refresh-token"));
  EXPECT_CALL(*session_mock_, GetSessionId())
      .WillRepeatedly(Return(test::kMockSessionId));

  EXPECT_CALL(device_name_service_mock_, ClearLastDeviceName()).Times(1);
  EXPECT_CALL(*token_cache_mock_, Clear()).Times(1);

  // Revoke token request after CancelAllRequests().
  {
    InSequence dummy;
    EXPECT_CALL(*request_manager_mock_, CancelAllRequests());
    EXPECT_CALL(*request_manager_mock_, StartRequest(_, _));
  }

  service_->EndSession(SER_USER_REQUESTED_LOGOUT);
}

TEST_F(AuthServiceImplTest, TokenRequestsAreDelayedUntilCacheLoads) {
  EXPECT_CALL(*session_mock_, Initialize(_));
  EXPECT_CALL(*session_mock_, LoadSession());
  EXPECT_CALL(*session_mock_, GetState())
      .WillRepeatedly(Return(OA2SS_IN_PROGRESS));
  EXPECT_CALL(*mock_oauth1_migrator_, EnsureOAuth1SessionIsCleared());
  EXPECT_CALL(*token_cache_mock_, Load(_));
  EXPECT_CALL(mock_session_state_observer_, OnSessionStateChanged());
  service_->Initialize(std::move(init_params_));

  MockAuthServiceClient mock_client;
  ScopeSet mock_scope({test::kMockScope4});
  service_->RegisterClient(&mock_client, test::kMockClientName1);
  service_->RequestAccessToken(&mock_client, mock_scope);

  // The token request is only processed upon OnTokenCacheLoaded().
  scoped_refptr<const AuthToken> mock_token(
      new AuthToken(test::kMockClientName1, test::kMockTokenValue1, mock_scope,
                    test::kMockValidExpirationTime));
  EXPECT_CALL(*token_cache_mock_,
              GetToken(test::kMockClientName1, ScopeSet({test::kMockScope4})))
      .WillOnce(Return(mock_token));
  RequestAccessTokenResponse response(OAE_UNSET, ScopeSet(), nullptr);
  EXPECT_CALL(mock_client, OnAccessTokenRequestCompleted(service_.get(), _))
      .WillOnce(::testing::SaveArg<1>(&response));
  service_->OnTokenCacheLoaded();

  EXPECT_EQ(OAE_OK, response.auth_error());
  EXPECT_EQ(mock_scope, response.scopes_requested());
  EXPECT_NE(nullptr, response.access_token().get());
  EXPECT_EQ(test::kMockClientName1, response.access_token()->client_name());
  EXPECT_EQ(test::kMockValidExpirationTime,
            response.access_token()->expiration_time());
  EXPECT_EQ(mock_scope, response.access_token()->scopes());
  EXPECT_EQ(test::kMockTokenValue1, response.access_token()->token());
}

TEST_F(AuthServiceImplTest, UsernameChangeAfterAuthErrorEndsSession) {
  EXPECT_CALL(*session_mock_, Initialize(_));
  EXPECT_CALL(*session_mock_, LoadSession());
  EXPECT_CALL(*session_mock_, GetState())
      .WillRepeatedly(Return(OA2SS_AUTH_ERROR));
  EXPECT_CALL(*session_mock_, HasAuthError()).WillOnce(Return(true));
  EXPECT_CALL(*session_mock_, GetUsername())
      .WillOnce(Return(test::kMockUsername));
  EXPECT_CALL(*session_mock_, GetSessionId())
      .WillRepeatedly(Return(test::kMockSessionId));
  EXPECT_CALL(*mock_oauth1_migrator_, EnsureOAuth1SessionIsCleared());
  EXPECT_CALL(*token_cache_mock_, Load(_));
  EXPECT_CALL(mock_session_state_observer_, OnSessionStateChanged());
  service_->Initialize(std::move(init_params_));
  service_->OnTokenCacheLoaded();

  // The interesting part: The session is terminated.
  EXPECT_CALL(*session_mock_, ClearSession()).Times(1);
  EXPECT_CALL(*session_mock_, StoreSession()).Times(1);

  EXPECT_CALL(device_name_service_mock_, ClearLastDeviceName()).Times(1);
  EXPECT_CALL(*token_cache_mock_, Clear()).Times(1);
  EXPECT_CALL(*request_manager_mock_, CancelAllRequests()).Times(1);
#if DCHECK_IS_ON()
  EXPECT_CALL(*session_mock_, GetRefreshToken())
      .WillOnce(Return(std::string()));
#endif  // DCHECK_IS_ON()

  service_->StartSessionWithAuthToken(test::kMockOtherUsername,
                                      test::kMockAuthToken);
}

TEST_F(AuthServiceImplTest, EnteringAuthErrorCancelsAllRequestsButRevokeToken) {
  EXPECT_CALL(*session_mock_, Initialize(_));
  EXPECT_CALL(*session_mock_, LoadSession());
  EXPECT_CALL(*session_mock_, GetState())
      .WillRepeatedly(Return(OA2SS_IN_PROGRESS));
  EXPECT_CALL(*mock_oauth1_migrator_, EnsureOAuth1SessionIsCleared());
  EXPECT_CALL(*token_cache_mock_, Load(_));
  EXPECT_CALL(mock_session_state_observer_, OnSessionStateChanged());
  service_->Initialize(std::move(init_params_));
  service_->OnTokenCacheLoaded();

  MockAuthServiceClient mock_client;
  ScopeSet mock_scope({test::kMockScope4});
  service_->RegisterClient(&mock_client, test::kMockClientName1);

  EXPECT_CALL(*token_cache_mock_, GetToken(test::kMockClientName1, mock_scope))
      .WillOnce(Return(nullptr));
  EXPECT_CALL(*session_mock_, GetRefreshToken())
      .WillRepeatedly(Return(test::kMockRefreshToken));
  EXPECT_CALL(*session_mock_, GetSessionId())
      .WillRepeatedly(Return(test::kMockSessionId));
  EXPECT_CALL(*session_mock_, GetSessionIdForDiagnostics())
      .WillRepeatedly(Return(test::kMockSessionId));
  service_->RequestAccessToken(&mock_client, mock_scope);

  scoped_refptr<NetworkRequest> access_token_request;
  EXPECT_CALL(*request_manager_mock_, StartRequest(_, _))
      .WillOnce(::testing::SaveArg<0>(&access_token_request));

  test_task_runner_->FastForwardUntilNoTasksRemain();

  EXPECT_NE(nullptr, access_token_request.get());
  AccessTokenRequest* atr =
      static_cast<AccessTokenRequest*>(access_token_request.get());

  EXPECT_CALL(*session_mock_, GetRefreshToken())
      .WillRepeatedly(Return(test::kMockRefreshToken));
  {
    // What we do care about is that the revoke token request is not
    // immeditately cancelled by a late CancelAllRequests().
    InSequence s;
    EXPECT_CALL(*request_manager_mock_, CancelAllRequests());
    EXPECT_CALL(*request_manager_mock_, StartRequest(_, _));
  }

  // Trigger an AUTH_ERROR.
  EXPECT_EQ(RS_OK, atr->TryResponse(net::HTTP_UNAUTHORIZED,
                                    "{\"error\": \"invalid_grant\"}"));
  service_->OnNetworkRequestFinished(access_token_request, RS_OK);
}

TEST_F(AuthServiceImplTest, RevokeTokenRequestContents) {
  EXPECT_CALL(*token_cache_mock_, Load(_));
  EXPECT_CALL(*session_mock_, Initialize(_));
  EXPECT_CALL(*session_mock_, LoadSession());
  EXPECT_CALL(*session_mock_, GetState())
      .WillRepeatedly(Return(OA2SS_IN_PROGRESS));

  service_->Initialize(std::move(init_params_));
  service_->OnTokenCacheLoaded();

  EXPECT_CALL(*session_mock_, SetState(OA2SS_FINISHING));
  EXPECT_CALL(*session_mock_, ClearSession());
  EXPECT_CALL(*session_mock_, StoreSession());
  EXPECT_CALL(*session_mock_, GetRefreshToken())
      .WillRepeatedly(Return("mock-refresh-token"));
  EXPECT_CALL(*session_mock_, GetSessionId())
      .WillRepeatedly(Return(test::kMockSessionId));
  EXPECT_CALL(*session_mock_, GetSessionIdForDiagnostics())
      .WillRepeatedly(Return(test::kMockSessionId));

  EXPECT_CALL(device_name_service_mock_, ClearLastDeviceName()).Times(1);
  EXPECT_CALL(*token_cache_mock_, Clear()).Times(1);
  // Revoke token request
  scoped_refptr<NetworkRequest> request;
  EXPECT_CALL(*request_manager_mock_, StartRequest(_, _))
      .WillOnce(::testing::SaveArg<0>(&request));
  EXPECT_CALL(*request_manager_mock_, CancelAllRequests());

  service_->EndSession(SER_USER_REQUESTED_LOGOUT);
  EXPECT_TRUE(request);

  RevokeTokenRequest* revoke_token_request =
      static_cast<RevokeTokenRequest*>(request.get());

  EXPECT_EQ(
      "client_id=fake-client-id-string&token=mock-refresh-token&token_type_"
      "hint=refresh_token",
      revoke_token_request->GetContent());
  EXPECT_EQ(std::vector<std::string>(),
            revoke_token_request->GetExtraRequestHeaders());
  EXPECT_EQ("sid=mock-session-id", revoke_token_request->GetQueryString());
}

TEST_F(AuthServiceImplTest, CanRequestAccessTokenInStateInProgress) {
  EXPECT_CALL(*token_cache_mock_, Load(_));
  EXPECT_CALL(*session_mock_, Initialize(_));
  EXPECT_CALL(*session_mock_, LoadSession());
  EXPECT_CALL(*session_mock_, GetState())
      .WillRepeatedly(Return(OA2SS_IN_PROGRESS));
  EXPECT_CALL(*mock_oauth1_migrator_, EnsureOAuth1SessionIsCleared());
  EXPECT_CALL(mock_session_state_observer_, OnSessionStateChanged());

  service_->Initialize(std::move(init_params_));
  service_->OnTokenCacheLoaded();

  MockAuthServiceClient mock_client;
  service_->RegisterClient(&mock_client, test::kMockClientName1);

  ScopeSet mock_scope({test::kMockScope4});
  service_->RequestAccessToken(&mock_client, mock_scope);
  EXPECT_CALL(*token_cache_mock_, GetToken(test::kMockClientName1, mock_scope))
      .WillOnce(Return(test::kValidToken1));

  RequestAccessTokenResponse response(OAE_UNSET, ScopeSet(), nullptr);
  EXPECT_CALL(mock_client, OnAccessTokenRequestCompleted(service_.get(), _))
      .WillOnce(::testing::SaveArg<1>(&response));
  test_task_runner_->FastForwardUntilNoTasksRemain();
}

TEST_F(AuthServiceImplTest, CantRequestAccessTokenInStateAuthError) {
  EXPECT_CALL(*token_cache_mock_, Load(_));
  EXPECT_CALL(*session_mock_, Initialize(_));
  EXPECT_CALL(*session_mock_, LoadSession());
  EXPECT_CALL(*session_mock_, GetState())
      .WillRepeatedly(Return(OA2SS_AUTH_ERROR));
  EXPECT_CALL(*mock_oauth1_migrator_, EnsureOAuth1SessionIsCleared());
  EXPECT_CALL(mock_session_state_observer_, OnSessionStateChanged());

  service_->Initialize(std::move(init_params_));
  service_->OnTokenCacheLoaded();

  MockAuthServiceClient mock_client;
  service_->RegisterClient(&mock_client, test::kMockClientName1);

  ScopeSet mock_scope({test::kMockScope4});
  EXPECT_CALL(mock_client,
              OnAccessTokenRequestDenied(service_.get(), mock_scope));
  service_->RequestAccessToken(&mock_client, mock_scope);

  EXPECT_EQ(0u, test_task_runner_->GetPendingTaskCount());
}

TEST_F(AuthServiceImplTest, CantRequestAccessTokenInStateStarting) {
  EXPECT_CALL(*token_cache_mock_, Load(_));
  EXPECT_CALL(*session_mock_, Initialize(_));
  EXPECT_CALL(*session_mock_, LoadSession());
  EXPECT_CALL(*session_mock_, GetState())
      .WillOnce(Return(OA2SS_IN_PROGRESS))
      .WillOnce(Return(OA2SS_IN_PROGRESS))
      .WillRepeatedly(Return(OA2SS_STARTING));
  EXPECT_CALL(*mock_oauth1_migrator_, EnsureOAuth1SessionIsCleared());
  EXPECT_CALL(mock_session_state_observer_, OnSessionStateChanged());

  service_->Initialize(std::move(init_params_));
  service_->OnTokenCacheLoaded();

  MockAuthServiceClient mock_client;
  service_->RegisterClient(&mock_client, test::kMockClientName1);

  ScopeSet mock_scope({test::kMockScope4});
  EXPECT_CALL(mock_client,
              OnAccessTokenRequestDenied(service_.get(), mock_scope));
  service_->RequestAccessToken(&mock_client, mock_scope);

  EXPECT_EQ(0u, test_task_runner_->GetPendingTaskCount());
}

TEST_F(AuthServiceImplTest, CantRequestAccessTokenInStateFinishing) {
  EXPECT_CALL(*token_cache_mock_, Load(_));
  EXPECT_CALL(*session_mock_, Initialize(_));
  EXPECT_CALL(*session_mock_, LoadSession());
  EXPECT_CALL(*session_mock_, GetState())
      .WillOnce(Return(OA2SS_IN_PROGRESS))
      .WillOnce(Return(OA2SS_IN_PROGRESS))
      .WillRepeatedly(Return(OA2SS_FINISHING));
  EXPECT_CALL(*mock_oauth1_migrator_, EnsureOAuth1SessionIsCleared());
  EXPECT_CALL(mock_session_state_observer_, OnSessionStateChanged());

  service_->Initialize(std::move(init_params_));
  service_->OnTokenCacheLoaded();

  MockAuthServiceClient mock_client;
  service_->RegisterClient(&mock_client, test::kMockClientName1);

  ScopeSet mock_scope({test::kMockScope4});
  EXPECT_CALL(mock_client,
              OnAccessTokenRequestDenied(service_.get(), mock_scope));
  service_->RequestAccessToken(&mock_client, mock_scope);

  EXPECT_EQ(0u, test_task_runner_->GetPendingTaskCount());
}

TEST_F(
    AuthServiceImplTest,
    CantRequestAccessTokenInStateAuthErrorIfStateChangedDuringRequestDefferred) {
  EXPECT_CALL(*token_cache_mock_, Load(_));
  EXPECT_CALL(*session_mock_, Initialize(_));
  EXPECT_CALL(*session_mock_, LoadSession());
  EXPECT_CALL(*session_mock_, GetState())
      .WillRepeatedly(Return(OA2SS_IN_PROGRESS));
  EXPECT_CALL(*mock_oauth1_migrator_, EnsureOAuth1SessionIsCleared());
  EXPECT_CALL(mock_session_state_observer_, OnSessionStateChanged());

  service_->Initialize(std::move(init_params_));
  service_->OnTokenCacheLoaded();

  MockAuthServiceClient mock_client;
  service_->RegisterClient(&mock_client, test::kMockClientName1);

  ScopeSet mock_scope({test::kMockScope4});
  service_->RequestAccessToken(&mock_client, mock_scope);
  EXPECT_EQ(1u, test_task_runner_->GetPendingTaskCount());

  EXPECT_CALL(*session_mock_, GetState())
      .WillRepeatedly(Return(OA2SS_AUTH_ERROR));
  EXPECT_CALL(mock_client,
              OnAccessTokenRequestDenied(service_.get(), mock_scope));
  test_task_runner_->FastForwardUntilNoTasksRemain();
  EXPECT_EQ(0u, test_task_runner_->GetPendingTaskCount());
}

TEST_F(AuthServiceImplTest, RefreshTokenAuthErrorCausesSessionAuthError) {
  EXPECT_CALL(*token_cache_mock_, Load(_));
  EXPECT_CALL(*session_mock_, Initialize(_));
  EXPECT_CALL(*session_mock_, LoadSession());
  EXPECT_CALL(*session_mock_, GetState())
      .WillRepeatedly(Return(OA2SS_INACTIVE));
  EXPECT_CALL(*mock_oauth1_migrator_, IsMigrationPossible())
      .WillOnce(Return(false));
  EXPECT_CALL(*session_mock_, SetState(OA2SS_INACTIVE));
  EXPECT_CALL(*session_mock_, SetState(OA2SS_STARTING));
  EXPECT_CALL(*session_mock_, GetSessionId())
      .WillRepeatedly(Return(test::kMockSessionId));
  EXPECT_CALL(*session_mock_, SetState(OA2SS_AUTH_ERROR));
  EXPECT_CALL(*session_mock_, SetStartMethod(SSM_AUTH_TOKEN));

  EXPECT_CALL(*session_mock_, SetUsername("mock-username"));

  EXPECT_CALL(device_name_service_mock_, HasDeviceNameChanged())
      .WillOnce(Return(false));

  scoped_refptr<NetworkRequest> access_token_request;
  EXPECT_CALL(*request_manager_mock_, StartRequest(_, _))
      .WillOnce(::testing::SaveArg<0>(&access_token_request));

  service_->Initialize(std::move(init_params_));
  service_->OnTokenCacheLoaded();

  service_->StartSessionWithAuthToken(test::kMockUsername,
                                      test::kMockAuthToken);

  EXPECT_NE(nullptr, access_token_request.get());
  AccessTokenRequest* atr =
      static_cast<AccessTokenRequest*>(access_token_request.get());

  EXPECT_EQ(RS_OK, atr->TryResponse(401, "{\"error\": \"invalid_grant\"}"));
  EXPECT_CALL(*session_mock_, GetStartMethod())
      .WillOnce(Return(SSM_AUTH_TOKEN));
  EXPECT_CALL(*session_mock_, StoreSession());
  EXPECT_CALL(*token_cache_mock_, Clear());

  service_->OnNetworkRequestFinished(access_token_request, RS_OK);
}

}  // namespace oauth2
}  // namespace opera
