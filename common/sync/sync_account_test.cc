// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sync_account_impl.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#include "common/account/account_service_delegate.h"
#include "common/account/mock_account_service.h"
#include "common/account/time_skew_resolver_mock.h"
#include "common/sync/sync_login_data_store.h"
#include "common/sync/sync_types.h"

#if defined(OPERA_DESKTOP)
#include "base/features/scoped_test_feature_override.h"
#include "desktop/common/features/features.h"
#endif

namespace opera {
namespace {

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class MockSyncLoginDataStore : public SyncLoginDataStore {
 public:
  MockSyncLoginDataStore() {
    ON_CALL(*this, LoadLoginData()).WillByDefault(Return(SyncLoginData()));
  }

  MOCK_CONST_METHOD0(LoadLoginData, SyncLoginData());
  MOCK_METHOD1(SaveLoginData, void(const SyncLoginData& login_data));
  MOCK_METHOD0(ClearSessionAndTokenData, void());
};

class FakeAccountAuthDataFetcher : public AccountAuthDataFetcher {
 public:
  typedef base::Callback<void(const RequestAuthDataSuccessCallback&,
                              const RequestAuthDataFailureCallback&)>
      OnRequest;

  explicit FakeAccountAuthDataFetcher(OnRequest on_request)
      : on_request_(on_request) {}

  void RequestAuthData(
      opera_sync::OperaAuthProblem problem,
      const RequestAuthDataSuccessCallback& success_callback,
      const RequestAuthDataFailureCallback& failure_callback) override {
    on_request_.Run(success_callback, failure_callback);
  }

  OnRequest on_request_;
};

class SyncAccountTest : public ::testing::Test {
 public:
  enum Result {
    RESULT_UNKNOWN, RESULT_SUCCESS, RESULT_FAILURE,
  };

  SyncAccountTest()
      : auth_data_request_result_(RESULT_UNKNOWN),
        syncing_stopped_(false),
        service_(nullptr),
        time_skew_resolver_(nullptr),
        service_delegate_(nullptr),
        data_store_(new NiceMock<MockSyncLoginDataStore>),
        account_(
            service_factory(),
            std::unique_ptr<TimeSkewResolver>(CreateTimeSkewResolver()),
            std::unique_ptr<SyncLoginDataStore>(data_store_),
            nullptr,
            updater_factory(),
            base::Bind(&SyncAccountTest::StopSyncing, base::Unretained(this)),
            opera::ExternalCredentials()),
        updater_created_(false),
        success_callback_(),
        failure_callback_() {
    logged_out_data_.user_name = "uuu";

    logged_in_data_.user_name = "nnn";
    logged_in_data_.auth_data.token = "ttt";
    logged_in_data_.auth_data.token_secret = "sss";
  }

 protected:
  SyncAccountImpl::ServiceFactory service_factory() {
    return base::Bind(&SyncAccountTest::CreateService, base::Unretained(this));
  }

  SyncAccountImpl::AuthDataUpdaterFactory updater_factory() {
    return base::Bind(&SyncAccountTest::CreateUpdater,
                      base::Unretained(this));
  }

  bool auth_data_requested() {
    return !success_callback_.is_null() && !failure_callback_.is_null();
  }

  AccountServiceDelegate::RequestAuthDataSuccessCallback
      auth_data_success_callback() {
    return base::Bind(&SyncAccountTest::AuthDataRequestSuccessful,
                      base::Unretained(this));
  }

  AccountServiceDelegate::RequestAuthDataFailureCallback
      auth_data_failure_callback() {
    return base::Bind(&SyncAccountTest::AuthDataRequestFailed,
                      base::Unretained(this));
  }

  TimeSkewResolver* CreateTimeSkewResolver() {
    time_skew_resolver_ = new TimeSkewResolverMock;
    return time_skew_resolver_;
  }

  SyncLoginData logged_out_data_;
  SyncLoginData logged_in_data_;
  Result auth_data_request_result_;
  bool syncing_stopped_;

  NiceMock<MockAccountService>* service_;
  TimeSkewResolverMock* time_skew_resolver_;
  AccountServiceDelegate* service_delegate_;
  NiceMock<MockSyncLoginDataStore>* data_store_;
  SyncAccountImpl account_;

  bool updater_created_;
  AccountServiceDelegate::RequestAuthDataSuccessCallback success_callback_;
  AccountServiceDelegate::RequestAuthDataFailureCallback failure_callback_;

 private:
  AccountService* CreateService(AccountServiceDelegate* delegate) {
    service_delegate_ = delegate;
    service_ = new NiceMock<MockAccountService>;
    return service_;
  }

  AccountAuthDataFetcher* CreateUpdater(const AccountAuthData& old_data) {
    EXPECT_EQ(logged_in_data_.auth_data, old_data);
    updater_created_ = true;
    return new FakeAccountAuthDataFetcher(base::Bind(
        &SyncAccountTest::OnRequestAuthData, base::Unretained(this)));
  }

  void AuthDataRequestSuccessful(const AccountAuthData& auth_data,
                                 opera_sync::OperaAuthProblem problem) {
    EXPECT_EQ(logged_in_data_.auth_data, auth_data);
    auth_data_request_result_ = RESULT_SUCCESS;
  }

  void AuthDataRequestFailed(account_util::AuthDataUpdaterError error_code,
                             account_util::AuthOperaComError auth_error,
                             const std::string& message,
                             opera_sync::OperaAuthProblem problem) {
    auth_data_request_result_ = RESULT_FAILURE;
  }

  void StopSyncing() {
    syncing_stopped_ = true;
    service_delegate_->LoggedOut();
  }

  void OnRequestAuthData(
      const AccountServiceDelegate::RequestAuthDataSuccessCallback&
          success_callback,
      const AccountServiceDelegate::RequestAuthDataFailureCallback&
          failure_callback) {
    success_callback_ = success_callback;
    failure_callback_ = failure_callback;
  }
};


TEST_F(SyncAccountTest, IsLoggedInIfInitializedWithAuthData) {
  auto time_skew_resolver = base::WrapUnique(CreateTimeSkewResolver());
  auto logged_in_store = base::WrapUnique(new MockSyncLoginDataStore);
  EXPECT_CALL(*logged_in_store, LoadLoginData())
      .WillOnce(Return(logged_in_data_));
  SyncAccountImpl logged_in_account(service_factory(),
                                    std::move(time_skew_resolver),
                                    std::move(logged_in_store),
                                    nullptr,
                                    updater_factory(),
                                    base::Closure(),
                                    opera::ExternalCredentials());
  EXPECT_TRUE(logged_in_account.IsLoggedIn());
}

TEST_F(SyncAccountTest, IsLoggedInIfGivenAuthData) {
  // Just logging-in must not save the Auth data.  It should only be saved
  // when synchronization becomes actually enabled (we get a successful
  // response from the Sync server).
  EXPECT_CALL(*data_store_, SaveLoginData(_)).Times(0);

  EXPECT_FALSE(account_.IsLoggedIn());

  account_.SetLoginData(logged_out_data_);
  EXPECT_FALSE(account_.IsLoggedIn());

  account_.SetLoginData(logged_in_data_);
  EXPECT_TRUE(account_.IsLoggedIn());
}

TEST_F(SyncAccountTest, IsLoggedOutOnLoginError) {
  account_.SetLoginData(logged_in_data_);
  account_.SetLoginErrorData(SyncLoginErrorData());
  EXPECT_FALSE(account_.IsLoggedIn());
}

TEST_F(SyncAccountTest, LogOutCallsService) {
  EXPECT_CALL(*service_, LogOut(account_util::LR_FOR_TEST_ONLY));

  account_.SetLoginData(logged_in_data_);
  account_.LogOut(account_util::LR_FOR_TEST_ONLY);
}

TEST_F(SyncAccountTest, IsLoggedOutIfCalledByService) {
  account_.SetLoginData(logged_in_data_);

  service_delegate_->LoggedOut();
  EXPECT_FALSE(account_.IsLoggedIn());
}

TEST_F(SyncAccountTest, ReturnsAuthDataIfAvailable) {
  account_.SetLoginData(logged_in_data_);
  opera_sync::OperaAuthProblem dummy;
  service_delegate_->RequestAuthData(dummy,
                                     auth_data_success_callback(),
                                     auth_data_failure_callback());
  EXPECT_EQ(RESULT_SUCCESS, auth_data_request_result_);
}

TEST_F(SyncAccountTest, StoresAuthDataWhenSyncEnabled) {
  EXPECT_CALL(*data_store_, SaveLoginData(_));

  account_.SetLoginData(logged_in_data_);
  account_.SyncEnabled();
}

TEST_F(SyncAccountTest, ClearsAuthDataOnLoginError) {
  EXPECT_CALL(*data_store_, ClearSessionAndTokenData());

  account_.SetLoginData(logged_in_data_);
  account_.SetLoginErrorData(SyncLoginErrorData());
}

TEST_F(SyncAccountTest, ClearsAuthDataIfCalledByServie) {
  EXPECT_CALL(*data_store_, ClearSessionAndTokenData());

  account_.SetLoginData(logged_in_data_);
  service_delegate_->LoggedOut();
}

TEST_F(SyncAccountTest, UpdatesAuthDataIfExpired) {
  account_.SetLoginData(logged_in_data_);
  time_skew_resolver_->SetResultValue(TimeSkewResolver::ResultValue(
      TimeSkewResolver::QueryResult::TIME_OK, 0, ""));
  service_delegate_->InvalidateAuthData();
  opera_sync::OperaAuthProblem dummy;
  service_delegate_->RequestAuthData(dummy,
                                     auth_data_success_callback(),
                                     auth_data_failure_callback());

  ASSERT_TRUE(updater_created_);
  ASSERT_TRUE(auth_data_requested());
  ASSERT_EQ(0, account_.login_data().time_skew);

  logged_in_data_.auth_data.token += " new";
  logged_in_data_.auth_data.token_secret += " new";
  EXPECT_CALL(*data_store_, SaveLoginData(logged_in_data_));
  success_callback_.Run(logged_in_data_.auth_data, dummy);

  EXPECT_EQ(RESULT_SUCCESS, auth_data_request_result_);
  EXPECT_TRUE(account_.IsLoggedIn());
  EXPECT_EQ(logged_in_data_.auth_data, account_.login_data().auth_data);
}

TEST_F(SyncAccountTest, DoesntUpdateAuthDataIfExpiredWithTimeSkew) {
  account_.SetLoginData(logged_in_data_);
  time_skew_resolver_->SetResultValue(TimeSkewResolver::ResultValue(
      TimeSkewResolver::QueryResult::TIME_SKEW, 1000000, "time skew"));
  service_delegate_->InvalidateAuthData();
  opera_sync::OperaAuthProblem dummy;
  service_delegate_->RequestAuthData(dummy,
                                     auth_data_success_callback(),
                                     auth_data_failure_callback());

  ASSERT_TRUE(updater_created_);
  ASSERT_FALSE(auth_data_requested());
  ASSERT_EQ(-1000000, account_.login_data().time_skew);

  EXPECT_EQ(RESULT_SUCCESS, auth_data_request_result_);
  EXPECT_TRUE(account_.IsLoggedIn());
  EXPECT_EQ(logged_in_data_.auth_data, account_.login_data().auth_data);
}


TEST_F(SyncAccountTest, UpdatesAuthDataIfExpiredWithInvalidTimeSkew) {
  account_.SetLoginData(logged_in_data_);
  time_skew_resolver_->SetResultValue(TimeSkewResolver::ResultValue(
      TimeSkewResolver::QueryResult::INVALID_RESPONSE_ERROR, 0, "time skew"));
  service_delegate_->InvalidateAuthData();
  opera_sync::OperaAuthProblem dummy;
  service_delegate_->RequestAuthData(dummy,
                                     auth_data_success_callback(),
                                     auth_data_failure_callback());

  ASSERT_TRUE(updater_created_);
  ASSERT_TRUE(auth_data_requested());
  ASSERT_EQ(0, account_.login_data().time_skew);

  logged_in_data_.auth_data.token += " new";
  logged_in_data_.auth_data.token_secret += " new";
  EXPECT_CALL(*data_store_, SaveLoginData(logged_in_data_));
  success_callback_.Run(logged_in_data_.auth_data, dummy);

  EXPECT_EQ(RESULT_SUCCESS, auth_data_request_result_);
  EXPECT_TRUE(account_.IsLoggedIn());
  EXPECT_EQ(logged_in_data_.auth_data, account_.login_data().auth_data);
}

TEST_F(SyncAccountTest, UpdateAuthDataIfTimeSkewRequestFailed) {
  account_.SetLoginData(logged_in_data_);
  time_skew_resolver_->SetResultValue(TimeSkewResolver::ResultValue(
      TimeSkewResolver::QueryResult::BAD_REQUEST_ERROR, 0, "error"));
  service_delegate_->InvalidateAuthData();
  opera_sync::OperaAuthProblem dummy;
  service_delegate_->RequestAuthData(dummy,
                                     auth_data_success_callback(),
                                     auth_data_failure_callback());

  ASSERT_TRUE(updater_created_);
  ASSERT_TRUE(auth_data_requested());
  ASSERT_EQ(0, account_.login_data().time_skew);

  logged_in_data_.auth_data.token += " new";
  logged_in_data_.auth_data.token_secret += " new";
  EXPECT_CALL(*data_store_, SaveLoginData(logged_in_data_));
  success_callback_.Run(logged_in_data_.auth_data, dummy);

  EXPECT_EQ(RESULT_SUCCESS, auth_data_request_result_);
  EXPECT_TRUE(account_.IsLoggedIn());
  EXPECT_EQ(logged_in_data_.auth_data, account_.login_data().auth_data);
}

TEST_F(SyncAccountTest, DoesNotStopSyncingOnFailureToUpdateAuthData) {
  account_.SetLoginData(logged_in_data_);
  service_delegate_->InvalidateAuthData();
  opera_sync::OperaAuthProblem dummy;
  service_delegate_->RequestAuthData(dummy,
                                     auth_data_success_callback(),
                                     auth_data_failure_callback());

  ASSERT_TRUE(updater_created_);
  ASSERT_TRUE(auth_data_requested());
  failure_callback_.Run(account_util::ADUE_NETWORK_ERROR,
      account_util::AOCE_420_NOT_AUTHORIZED_REQUEST,
      "A serious network failure.",
      dummy);

  EXPECT_EQ(RESULT_FAILURE, auth_data_request_result_);
  EXPECT_TRUE(account_.IsLoggedIn());

  EXPECT_FALSE(syncing_stopped_);
}

TEST_F(SyncAccountTest, SameTokenRenewalWITHSmartToken) {
  opera_sync::OperaAuthProblem auth_problem;
  auth_problem.set_auth_errcode(401);
  auth_problem.set_source(opera_sync::OperaAuthProblem::SOURCE_SYNC);
  auth_problem.set_token("sum_token");

  account_.SetLoginData(logged_in_data_);
  EXPECT_CALL(*data_store_, SaveLoginData(logged_in_data_)).Times(2);

  service_delegate_->InvalidateAuthData();

  service_delegate_->RequestAuthData(auth_problem,
      auth_data_success_callback(),
      auth_data_failure_callback());
  ASSERT_TRUE(updater_created_);
  ASSERT_TRUE(auth_data_requested());
  success_callback_.Run(logged_in_data_.auth_data, auth_problem);

  EXPECT_EQ(RESULT_SUCCESS, auth_data_request_result_);
  EXPECT_TRUE(account_.IsLoggedIn());
  EXPECT_EQ(logged_in_data_.auth_data, account_.login_data().auth_data);

  // Next attempt results in success, success callback is called automatically.
  service_delegate_->InvalidateAuthData();
  service_delegate_->RequestAuthData(auth_problem,
      auth_data_success_callback(),
      auth_data_failure_callback());

  EXPECT_EQ(RESULT_SUCCESS, auth_data_request_result_);
  EXPECT_TRUE(account_.IsLoggedIn());
  EXPECT_EQ(logged_in_data_.auth_data, account_.login_data().auth_data);
}

}  // namespace
}  // namespace opera
