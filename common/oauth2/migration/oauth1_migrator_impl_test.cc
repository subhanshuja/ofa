// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/migration/oauth1_migrator_impl.h"

#include <memory>
#include <string>

#include "base/json/json_writer.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_util.h"
#include "components/prefs/overlay_user_pref_store.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/prefs/testing_pref_store.h"
#include "components/syncable_prefs/pref_service_mock_factory.h"
#include "components/syncable_prefs/pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "common/oauth2/device_name/device_name_service_mock.h"
#include "common/oauth2/network/migration_token_request.h"
#include "common/oauth2/network/migration_token_response.h"
#include "common/oauth2/network/network_request_manager_mock.h"
#include "common/oauth2/network/oauth1_renew_token_request.h"
#include "common/oauth2/network/oauth1_renew_token_response.h"
#include "common/oauth2/session/persistent_session_mock.h"
#include "common/oauth2/test/testing_constants.h"
#include "common/sync/sync_login_data_store_mock.h"

namespace opera {

namespace oauth2 {

namespace {

using ::testing::_;
using ::testing::AtMost;
using ::testing::Return;

const GURL kMockOAuth1BaseUrl("http://mock.oauth1.url");
const GURL kMockOAuth2BaseUrl("http://mock.oauth2.url");

const char kMockOAuth1ServiceId[] = "mock-auth-service-id";
const char kMockOAuth1ClientId[] = "mock-oauth1-client-id";
const char kMockOAuth1ClientSecret[] = "mock-oauth1-client-secret";
const char kMockOAuth2ClientId[] = "mock-oauth2-client-id";

class OAuth1MigratorImplTest : public ::testing::Test {
 public:
  OAuth1MigratorImplTest() {}
  ~OAuth1MigratorImplTest() override {}

  void SetUp() override {
    ConfigurureJsonResponses();

    PersistentPrefStore* testing_user_prefs = new TestingPrefStore();
    OverlayUserPrefStore* overlay_user_prefs =
        new OverlayUserPrefStore(testing_user_prefs);

    syncable_prefs::PrefServiceMockFactory mock_pref_service_factory;
    mock_pref_service_factory.set_user_prefs(
        make_scoped_refptr(overlay_user_prefs));
    scoped_refptr<user_prefs::PrefRegistrySyncable> registry(
        new user_prefs::PrefRegistrySyncable);
    syncable_prefs::PrefServiceSyncable* prefs =
        mock_pref_service_factory.CreateSyncable(registry.get()).release();

    login_data_store_mock_ = new SyncLoginDataStoreMock;
    pref_service_ = prefs;
  }

  void InitMigrator() {
    oauth1_migrator_.reset(new OAuth1MigratorImpl(
        nullptr, &request_manager_mock_, &device_name_service_mock_,
        base::WrapUnique(login_data_store_mock_), kMockOAuth1BaseUrl,
        kMockOAuth2BaseUrl, kMockOAuth1ServiceId, kMockOAuth1ClientId,
        kMockOAuth1ClientSecret, kMockOAuth2ClientId));
  }

  void ConfigurureJsonResponses() {
    std::unique_ptr<base::DictionaryValue> token_response(
        new base::DictionaryValue);
    token_response->SetString("access_token", "mock-access-token");
    token_response->SetString("refresh_token", "mock-refresh-token");
    token_response->SetString("token_type", "Bearer");
    token_response->SetString("user_id", "mock-user-id");
    token_response->SetInteger("expires_in", 3601);
    ASSERT_TRUE(base::JSONWriter::Write(*token_response.get(),
                                        &valid_token_response_json_));

    token_response.reset(new base::DictionaryValue);
    token_response->SetString("error", "invalid_grant");
    ASSERT_TRUE(base::JSONWriter::Write(*token_response.get(),
                                        &expired_token_response_json_));

    token_response.reset(new base::DictionaryValue);
    token_response->SetString("auth_token", "mock_oauth1_token");
    token_response->SetString("auth_token_secret", "mock_oauth1_token_secret");
    token_response->SetString("userName", "mock_oauth1_username");
    ASSERT_TRUE(base::JSONWriter::Write(*token_response.get(),
                                        &oauth1_renew_token_success_json_));

    token_response.reset(new base::DictionaryValue);
    token_response->SetInteger("err_code", 428);
    token_response->SetString("err_msg", "Token doesn't need renewal");
    ASSERT_TRUE(base::JSONWriter::Write(*token_response.get(),
                                        &oauth1_renew_token_428_json_));

    token_response.reset(new base::DictionaryValue);
    token_response->SetInteger("err_code", 425);
    token_response->SetString("err_msg", "General Major Error 425");
    ASSERT_TRUE(base::JSONWriter::Write(*token_response.get(),
                                        &oauth1_renew_token_425_json_));
  }

  void TearDown() override {}

 protected:
  std::unique_ptr<OAuth1MigratorImpl> oauth1_migrator_;

  NetworkRequestManagerMock request_manager_mock_;
  PersistentSessionMock session_mock_;
  PrefService* pref_service_;
  SyncLoginDataStoreMock* login_data_store_mock_;
  DeviceNameServiceMock device_name_service_mock_;

  std::string expired_token_response_json_;
  std::string valid_token_response_json_;
  std::string oauth1_renew_token_success_json_;
  std::string oauth1_renew_token_428_json_;
  std::string oauth1_renew_token_425_json_;
};

}  // namespace

TEST_F(OAuth1MigratorImplTest, DiagnosticSnapshotContents) {
  SyncLoginData empty_login_data;
  EXPECT_CALL(*login_data_store_mock_, LoadLoginData())
      .WillOnce(Return(empty_login_data));

  InitMigrator();
  ASSERT_EQ("oauth1_migrator", oauth1_migrator_->GetDiagnosticName());
  const auto& snapshot = oauth1_migrator_->GetDiagnosticSnapshot();

  std::unique_ptr<base::DictionaryValue> expected(new base::DictionaryValue);
  expected->SetBoolean("migration_possible", false);
  expected->SetBoolean("migration_token_request_pending", false);
  expected->SetBoolean("oauth1_renew_token_request_pending", false);
  expected->SetString("migration_result", "UNSET");

  EXPECT_TRUE(snapshot->Equals(expected.get())) << "Actual: " << *snapshot
                                                << "\nExpected: " << *expected;
}

TEST_F(OAuth1MigratorImplTest, MigrationNotPossibleWithEmptyOAuth1Data) {
  SyncLoginData empty_login_data;
  EXPECT_CALL(*login_data_store_mock_, LoadLoginData())
      .WillOnce(Return(empty_login_data));
  InitMigrator();
  ASSERT_FALSE(oauth1_migrator_->IsMigrationPossible());
  ASSERT_EQ(MR_UNSET, oauth1_migrator_->GetMigrationResult());
}

TEST_F(OAuth1MigratorImplTest,
       MigrationNotPossibleWithIncompleteOAuth1DataNoLogin) {
  SyncLoginData incomplete_login_data;
  incomplete_login_data.auth_data.token = "fake-oauth1-token";
  incomplete_login_data.auth_data.token_secret = "fake-oauth1-token";
  incomplete_login_data.user_id = "fake-user-id";
  EXPECT_CALL(*login_data_store_mock_, LoadLoginData())
      .WillOnce(Return(incomplete_login_data));
  InitMigrator();
  ASSERT_FALSE(oauth1_migrator_->IsMigrationPossible());
  ASSERT_EQ(MR_UNSET, oauth1_migrator_->GetMigrationResult());
}

TEST_F(OAuth1MigratorImplTest,
       MigrationNotPossibleWithIncompleteOAuth1DataNoUserEmail) {
  SyncLoginData incomplete_login_data;
  incomplete_login_data.auth_data.token = "fake-oauth1-token";
  incomplete_login_data.auth_data.token_secret = "fake-oauth1-token";
  incomplete_login_data.user_id = "fake-user-id";
  incomplete_login_data.used_username_to_login = false;
  EXPECT_CALL(*login_data_store_mock_, LoadLoginData())
      .WillOnce(Return(incomplete_login_data));
  InitMigrator();
  ASSERT_FALSE(oauth1_migrator_->IsMigrationPossible());
  ASSERT_EQ(MR_UNSET, oauth1_migrator_->GetMigrationResult());
}

TEST_F(OAuth1MigratorImplTest, MigrationPossible) {
  SyncLoginData complete_login_data;
  complete_login_data.auth_data.token = "fake-oauth1-token";
  complete_login_data.auth_data.token_secret = "fake-oauth1-token";
  complete_login_data.user_id = "fake-user-id";
  complete_login_data.user_email = "fake-user-email";
  complete_login_data.used_username_to_login = false;
  EXPECT_CALL(*login_data_store_mock_, LoadLoginData())
      .WillOnce(Return(complete_login_data));
  InitMigrator();
  ASSERT_TRUE(oauth1_migrator_->IsMigrationPossible());
  ASSERT_EQ(MR_UNSET, oauth1_migrator_->GetMigrationResult());
}

TEST_F(OAuth1MigratorImplTest, MigrationPossibleWithoutOAuth1DataNoUserId) {
  SyncLoginData incomplete_login_data;
  incomplete_login_data.auth_data.token = "fake-oauth1-token";
  incomplete_login_data.auth_data.token_secret = "fake-oauth1-token";
  incomplete_login_data.user_email = "fake-user-email";
  incomplete_login_data.used_username_to_login = false;
  EXPECT_CALL(*login_data_store_mock_, LoadLoginData())
      .WillOnce(Return(incomplete_login_data));
  InitMigrator();
  ASSERT_TRUE(oauth1_migrator_->IsMigrationPossible());
  ASSERT_EQ(MR_UNSET, oauth1_migrator_->GetMigrationResult());
}

TEST_F(OAuth1MigratorImplTest, ClearsOAuth1Session) {
  SyncLoginData complete_login_data;
  complete_login_data.auth_data.token = "fake-oauth1-token";
  complete_login_data.auth_data.token_secret = "fake-oauth1-token";
  complete_login_data.user_email = "fake-user-email";
  complete_login_data.used_username_to_login = false;
  EXPECT_CALL(*login_data_store_mock_, LoadLoginData())
      .WillOnce(Return(complete_login_data));
  InitMigrator();
  ASSERT_TRUE(oauth1_migrator_->IsMigrationPossible());

  EXPECT_CALL(*login_data_store_mock_, ClearSessionAndTokenData());
  oauth1_migrator_->EnsureOAuth1SessionIsCleared();
  ASSERT_FALSE(oauth1_migrator_->IsMigrationPossible());
  ASSERT_EQ(MR_UNSET, oauth1_migrator_->GetMigrationResult());
}

TEST_F(OAuth1MigratorImplTest, StartMigrationOAuth1TokenNonExpired) {
  SyncLoginData complete_login_data;
  complete_login_data.auth_data.token = "fake-oauth1-token";
  complete_login_data.auth_data.token_secret = "fake-oauth1-token";
  complete_login_data.user_email = "fake-user-email";
  complete_login_data.used_username_to_login = false;
  EXPECT_CALL(*login_data_store_mock_, LoadLoginData())
      .WillOnce(Return(complete_login_data));
  InitMigrator();
  ASSERT_TRUE(oauth1_migrator_->IsMigrationPossible());

#if DCHECK_IS_ON()
  EXPECT_CALL(session_mock_, GetState())
      .WillOnce(Return(OA2SS_INACTIVE))
      .WillOnce(Return(OA2SS_STARTING));
  EXPECT_CALL(session_mock_, GetStartMethod()).WillOnce(Return(SSM_OAUTH1));
#endif  // DCHECK_IS_ON()
  EXPECT_CALL(session_mock_, GetSessionIdForDiagnostics())
      .WillRepeatedly(Return(test::kMockSessionId));
  EXPECT_CALL(session_mock_, SetStartMethod(SSM_OAUTH1)).Times(1);
  EXPECT_CALL(session_mock_, SetUsername("fake-user-email")).Times(1);
  EXPECT_CALL(session_mock_, SetState(OA2SS_STARTING)).Times(1);

  EXPECT_CALL(device_name_service_mock_, HasDeviceNameChanged())
      .WillOnce(Return(false));

  scoped_refptr<NetworkRequest> refresh_token_request;
  EXPECT_CALL(request_manager_mock_, StartRequest(_, _))
      .WillOnce(::testing::SaveArg<0>(&refresh_token_request));

  oauth1_migrator_->PrepareMigration(&session_mock_);
  oauth1_migrator_->StartMigration();
  EXPECT_TRUE(refresh_token_request);
  MigrationTokenRequest* migration_token_request =
      static_cast<MigrationTokenRequest*>(refresh_token_request.get());
  EXPECT_EQ("sid=mock-session-id", migration_token_request->GetQueryString());
  EXPECT_EQ(
      "client_id=mock-oauth2-client-id&"
      "grant_type=oauth1_token&scope=ALL",
      migration_token_request->GetContent());
  const auto& actual_extra_request_headers =
      migration_token_request->GetExtraRequestHeaders();
  ASSERT_EQ(1u, actual_extra_request_headers.size());
  EXPECT_TRUE(base::StartsWith(actual_extra_request_headers.at(0),
                               "Authorization:", base::CompareCase::SENSITIVE));

  EXPECT_EQ(RS_OK, migration_token_request->TryResponse(
                       200, valid_token_response_json_));

  EXPECT_CALL(session_mock_, SetRefreshToken("mock-refresh-token")).Times(1);
  EXPECT_CALL(session_mock_, SetUserId("mock-user-id")).Times(1);
  EXPECT_CALL(session_mock_, SetState(OA2SS_IN_PROGRESS)).Times(1);
  EXPECT_CALL(session_mock_, StoreSession()).Times(1);

  EXPECT_CALL(*login_data_store_mock_, ClearSessionAndTokenData());
  oauth1_migrator_->OnNetworkRequestFinished(refresh_token_request, RS_OK);
  ASSERT_EQ(MR_SUCCESS, oauth1_migrator_->GetMigrationResult());
}

TEST_F(OAuth1MigratorImplTest, StartMigrationOAuth1TokenExpiredCanRenew) {
  SyncLoginData complete_login_data;
  complete_login_data.auth_data.token = "fake-oauth1-token";
  complete_login_data.auth_data.token_secret = "fake-oauth1-token";
  complete_login_data.user_email = "fake-user-email";
  complete_login_data.used_username_to_login = false;
  EXPECT_CALL(*login_data_store_mock_, LoadLoginData())
      .WillOnce(Return(complete_login_data));
  InitMigrator();
  ASSERT_TRUE(oauth1_migrator_->IsMigrationPossible());

#if DCHECK_IS_ON()
  EXPECT_CALL(session_mock_, GetState())
      .WillOnce(Return(OA2SS_INACTIVE))
      .WillOnce(Return(OA2SS_STARTING));
  EXPECT_CALL(session_mock_, GetStartMethod()).WillOnce(Return(SSM_OAUTH1));
#endif  // DCHECK_IS_ON()
  EXPECT_CALL(session_mock_, GetSessionIdForDiagnostics())
      .WillRepeatedly(Return(test::kMockSessionId));

  EXPECT_CALL(session_mock_, SetStartMethod(SSM_OAUTH1)).Times(1);
  EXPECT_CALL(session_mock_, SetUsername("fake-user-email")).Times(1);
  EXPECT_CALL(session_mock_, SetState(OA2SS_STARTING)).Times(1);

  scoped_refptr<NetworkRequest> refresh_token_request;
  EXPECT_CALL(request_manager_mock_, StartRequest(_, _))
      .WillOnce(::testing::SaveArg<0>(&refresh_token_request));

  EXPECT_CALL(device_name_service_mock_, HasDeviceNameChanged())
      .WillRepeatedly(Return(false));
  oauth1_migrator_->PrepareMigration(&session_mock_);
  oauth1_migrator_->StartMigration();
  EXPECT_TRUE(refresh_token_request);
  MigrationTokenRequest* migration_token_request =
      static_cast<MigrationTokenRequest*>(refresh_token_request.get());
  EXPECT_EQ("sid=mock-session-id", migration_token_request->GetQueryString());
  EXPECT_EQ(
      "client_id=mock-oauth2-client-id&"
      "grant_type=oauth1_token&scope=ALL",
      migration_token_request->GetContent());
  const auto& actual_extra_request_headers =
      migration_token_request->GetExtraRequestHeaders();
  ASSERT_EQ(1u, actual_extra_request_headers.size());
  EXPECT_TRUE(base::StartsWith(actual_extra_request_headers.at(0),
                               "Authorization:", base::CompareCase::SENSITIVE));

  EXPECT_EQ(RS_OK, migration_token_request->TryResponse(
                       401, expired_token_response_json_));

  scoped_refptr<NetworkRequest> renew_token_request;
  EXPECT_CALL(request_manager_mock_, StartRequest(_, _))
      .WillOnce(::testing::SaveArg<0>(&renew_token_request));
  oauth1_migrator_->OnNetworkRequestFinished(refresh_token_request, RS_OK);
  OAuth1RenewTokenRequest* oauth1_renew_token_request =
      static_cast<OAuth1RenewTokenRequest*>(renew_token_request.get());
  EXPECT_EQ(
      "consumer_key=mock-oauth1-client-id&old_token=fake-oauth1-token&service="
      "mock-auth-service-id&signature="
      "a6816f977f8934ed0e89ca07bf1d95d8c0fd2788",
      oauth1_renew_token_request->GetQueryString());
  EXPECT_EQ(RS_OK, oauth1_renew_token_request->TryResponse(
                       200, oauth1_renew_token_success_json_));

  scoped_refptr<NetworkRequest> second_refresh_token_request;
  EXPECT_CALL(request_manager_mock_, StartRequest(_, _))
      .WillOnce(::testing::SaveArg<0>(&second_refresh_token_request));
  oauth1_migrator_->OnNetworkRequestFinished(renew_token_request, RS_OK);
  MigrationTokenRequest* second_migration_token_request =
      static_cast<MigrationTokenRequest*>(second_refresh_token_request.get());
  EXPECT_EQ(RS_OK, second_migration_token_request->TryResponse(
                       200, valid_token_response_json_));

  EXPECT_CALL(session_mock_, SetRefreshToken("mock-refresh-token")).Times(1);
  EXPECT_CALL(session_mock_, SetUserId("mock-user-id")).Times(1);
  EXPECT_CALL(session_mock_, SetState(OA2SS_IN_PROGRESS)).Times(1);
  EXPECT_CALL(session_mock_, StoreSession()).Times(1);

  EXPECT_CALL(*login_data_store_mock_, ClearSessionAndTokenData());
  oauth1_migrator_->OnNetworkRequestFinished(second_refresh_token_request,
                                             RS_OK);
  ASSERT_EQ(MR_SUCCESS_WITH_BOUNCE, oauth1_migrator_->GetMigrationResult());
}

TEST_F(OAuth1MigratorImplTest, StartMigrationOAuth1TokenExpiredBut428) {
  SyncLoginData complete_login_data;
  complete_login_data.auth_data.token = "fake-oauth1-token";
  complete_login_data.auth_data.token_secret = "fake-oauth1-token";
  complete_login_data.user_email = "fake-user-email";
  complete_login_data.used_username_to_login = false;
  EXPECT_CALL(*login_data_store_mock_, LoadLoginData())
      .WillOnce(Return(complete_login_data));
  InitMigrator();
  ASSERT_TRUE(oauth1_migrator_->IsMigrationPossible());

#if DCHECK_IS_ON()
  EXPECT_CALL(session_mock_, GetState())
      .WillOnce(Return(OA2SS_INACTIVE))
      .WillOnce(Return(OA2SS_STARTING));
  EXPECT_CALL(session_mock_, GetStartMethod()).WillOnce(Return(SSM_OAUTH1));
#endif  // DCHECK_IS_ON()
  EXPECT_CALL(session_mock_, GetSessionIdForDiagnostics())
      .WillRepeatedly(Return(test::kMockSessionId));

  EXPECT_CALL(session_mock_, SetStartMethod(SSM_OAUTH1)).Times(1);
  EXPECT_CALL(session_mock_, SetUsername("fake-user-email")).Times(1);
  EXPECT_CALL(session_mock_, SetState(OA2SS_STARTING)).Times(1);

  scoped_refptr<NetworkRequest> refresh_token_request;
  EXPECT_CALL(request_manager_mock_, StartRequest(_, _))
      .WillOnce(::testing::SaveArg<0>(&refresh_token_request));

  EXPECT_CALL(device_name_service_mock_, HasDeviceNameChanged())
      .WillRepeatedly(Return(false));
  oauth1_migrator_->PrepareMigration(&session_mock_);
  oauth1_migrator_->StartMigration();
  EXPECT_TRUE(refresh_token_request);
  MigrationTokenRequest* migration_token_request =
      static_cast<MigrationTokenRequest*>(refresh_token_request.get());
  EXPECT_EQ("sid=mock-session-id", migration_token_request->GetQueryString());
  EXPECT_EQ(
      "client_id=mock-oauth2-client-id&"
      "grant_type=oauth1_token&scope=ALL",
      migration_token_request->GetContent());
  const auto& actual_extra_request_headers =
      migration_token_request->GetExtraRequestHeaders();
  ASSERT_EQ(1u, actual_extra_request_headers.size());
  EXPECT_TRUE(base::StartsWith(actual_extra_request_headers.at(0),
                               "Authorization:", base::CompareCase::SENSITIVE));

  EXPECT_EQ(RS_OK, migration_token_request->TryResponse(
                       401, expired_token_response_json_));

  scoped_refptr<NetworkRequest> renew_token_request;
  EXPECT_CALL(request_manager_mock_, StartRequest(_, _))
      .WillOnce(::testing::SaveArg<0>(&renew_token_request));
  oauth1_migrator_->OnNetworkRequestFinished(refresh_token_request, RS_OK);
  OAuth1RenewTokenRequest* oauth1_renew_token_request =
      static_cast<OAuth1RenewTokenRequest*>(renew_token_request.get());
  EXPECT_EQ(
      "consumer_key=mock-oauth1-client-id&old_token=fake-oauth1-token&service="
      "mock-auth-service-id&signature="
      "a6816f977f8934ed0e89ca07bf1d95d8c0fd2788",
      oauth1_renew_token_request->GetQueryString());
  EXPECT_EQ(RS_OK, oauth1_renew_token_request->TryResponse(
                       200, oauth1_renew_token_428_json_));

  scoped_refptr<NetworkRequest> second_refresh_token_request;
  EXPECT_CALL(request_manager_mock_, StartRequest(_, _))
      .WillOnce(::testing::SaveArg<0>(&second_refresh_token_request));
  oauth1_migrator_->OnNetworkRequestFinished(renew_token_request, RS_OK);
  MigrationTokenRequest* second_migration_token_request =
      static_cast<MigrationTokenRequest*>(second_refresh_token_request.get());
  EXPECT_EQ(RS_OK, second_migration_token_request->TryResponse(
                       200, valid_token_response_json_));

  EXPECT_CALL(session_mock_, SetRefreshToken("mock-refresh-token")).Times(1);
  EXPECT_CALL(session_mock_, SetUserId("mock-user-id")).Times(1);
  EXPECT_CALL(session_mock_, SetState(OA2SS_IN_PROGRESS)).Times(1);
  EXPECT_CALL(session_mock_, StoreSession()).Times(1);

  EXPECT_CALL(*login_data_store_mock_, ClearSessionAndTokenData());
  oauth1_migrator_->OnNetworkRequestFinished(second_refresh_token_request,
                                             RS_OK);
  ASSERT_EQ(MR_SUCCESS_WITH_BOUNCE, oauth1_migrator_->GetMigrationResult());
}

TEST_F(OAuth1MigratorImplTest,
       StartMigrationOAuth1TokenExpiredButRenewalFails) {
  SyncLoginData complete_login_data;
  complete_login_data.auth_data.token = "fake-oauth1-token";
  complete_login_data.auth_data.token_secret = "fake-oauth1-token";
  complete_login_data.user_email = "fake-user-email";
  complete_login_data.used_username_to_login = false;
  EXPECT_CALL(*login_data_store_mock_, LoadLoginData())
      .WillOnce(Return(complete_login_data));
  InitMigrator();
  ASSERT_TRUE(oauth1_migrator_->IsMigrationPossible());

#if DCHECK_IS_ON()
  EXPECT_CALL(session_mock_, GetState())
      .WillOnce(Return(OA2SS_INACTIVE))
      .WillOnce(Return(OA2SS_STARTING));
  EXPECT_CALL(session_mock_, GetStartMethod()).WillOnce(Return(SSM_OAUTH1));
#endif  // DCHECK_IS_ON()
  EXPECT_CALL(session_mock_, GetSessionIdForDiagnostics())
      .WillRepeatedly(Return(test::kMockSessionId));

  EXPECT_CALL(session_mock_, SetStartMethod(SSM_OAUTH1)).Times(1);
  EXPECT_CALL(session_mock_, SetUsername("fake-user-email")).Times(1);
  EXPECT_CALL(session_mock_, SetState(OA2SS_STARTING)).Times(1);

  scoped_refptr<NetworkRequest> refresh_token_request;
  EXPECT_CALL(request_manager_mock_, StartRequest(_, _))
      .WillOnce(::testing::SaveArg<0>(&refresh_token_request));

  EXPECT_CALL(device_name_service_mock_, HasDeviceNameChanged())
      .WillOnce(Return(false));
  oauth1_migrator_->PrepareMigration(&session_mock_);
  oauth1_migrator_->StartMigration();
  EXPECT_TRUE(refresh_token_request);
  MigrationTokenRequest* migration_token_request =
      static_cast<MigrationTokenRequest*>(refresh_token_request.get());
  EXPECT_EQ("sid=mock-session-id", migration_token_request->GetQueryString());
  EXPECT_EQ(
      "client_id=mock-oauth2-client-id&"
      "grant_type=oauth1_token&scope=ALL",
      migration_token_request->GetContent());
  const auto& actual_extra_request_headers =
      migration_token_request->GetExtraRequestHeaders();
  ASSERT_EQ(1u, actual_extra_request_headers.size());
  EXPECT_TRUE(base::StartsWith(actual_extra_request_headers.at(0),
                               "Authorization:", base::CompareCase::SENSITIVE));

  EXPECT_EQ(RS_OK, migration_token_request->TryResponse(
                       401, expired_token_response_json_));

  scoped_refptr<NetworkRequest> renew_token_request;
  EXPECT_CALL(request_manager_mock_, StartRequest(_, _))
      .WillOnce(::testing::SaveArg<0>(&renew_token_request));
  oauth1_migrator_->OnNetworkRequestFinished(refresh_token_request, RS_OK);
  OAuth1RenewTokenRequest* oauth1_renew_token_request =
      static_cast<OAuth1RenewTokenRequest*>(renew_token_request.get());
  EXPECT_EQ(
      "consumer_key=mock-oauth1-client-id&old_token=fake-oauth1-token&service="
      "mock-auth-service-id&signature="
      "a6816f977f8934ed0e89ca07bf1d95d8c0fd2788",
      oauth1_renew_token_request->GetQueryString());
  EXPECT_EQ(RS_OK, oauth1_renew_token_request->TryResponse(
                       200, oauth1_renew_token_425_json_));

  EXPECT_CALL(session_mock_, SetState(OA2SS_AUTH_ERROR)).Times(1);
  EXPECT_CALL(*login_data_store_mock_, ClearSessionAndTokenData());
  oauth1_migrator_->OnNetworkRequestFinished(renew_token_request, RS_OK);
  ASSERT_EQ(MR_O1_425_INVALID_OPERA_TOKEN,
            oauth1_migrator_->GetMigrationResult());
}

}  // namespace oauth2

}  // namespace opera
