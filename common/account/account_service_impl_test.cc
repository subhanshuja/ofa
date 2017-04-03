// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/account/account_service_impl.h"

#include "net/base/net_errors.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#include "common/account/account_service_delegate.h"
#include "common/account/mock_account_observer.h"

namespace opera {
namespace {

struct SuccessfulAccountServiceDelegate : public AccountServiceDelegate {
 public:
  SuccessfulAccountServiceDelegate(const std::string& token,
                                   const std::string& token_secret)
      : logged_out_(false), auth_data_valid_(true) {
    auth_data_.token = token;
    auth_data_.token_secret = token_secret;
  }

  void RequestAuthData(
      opera_sync::OperaAuthProblem problem,
      const RequestAuthDataSuccessCallback& success_callback,
      const RequestAuthDataFailureCallback& failure_callback) override {
    success_callback.Run(auth_data_, problem);
  }
  void InvalidateAuthData() override { auth_data_valid_ = false; }
  void LoggedOut() override { logged_out_ = true; }

  const AccountAuthData& auth_data() const { return auth_data_; }
  bool auth_data_valid() const { return auth_data_valid_; }
  bool logged_out() const { return logged_out_; }

 private:
  AccountAuthData auth_data_;
  bool logged_out_;
  bool auth_data_valid_;
};

class FailingAccountServiceDelegate : public AccountServiceDelegate {
 public:
  FailingAccountServiceDelegate(account_util::AuthDataUpdaterError error_code,
                                account_util::AuthOperaComError auth_code,
                                const std::string& message)
      : error_code_(error_code), auth_code_(auth_code), message_(message) {}

  void RequestAuthData(
      opera_sync::OperaAuthProblem problem,
      const RequestAuthDataSuccessCallback& success_callback,
      const RequestAuthDataFailureCallback& failure_callback) override {
    failure_callback.Run(error_code_, auth_code_, message_, problem);
  }

  void InvalidateAuthData() override {}

  void LoggedOut() override {}

 private:
  account_util::AuthDataUpdaterError error_code_;
  account_util::AuthOperaComError auth_code_;
  std::string message_;
};

TEST(AccountServiceImplTest, AuthHeaderGeneratedCorrectly) {
  SuccessfulAccountServiceDelegate delegate("toktok", "shhhh");
  AccountServiceImpl account("clientkey", "clientsecret", &delegate);
  account.LogIn();

  const char base_url[] = "http://host.net/resource";
  const AccountService::HttpMethod method = AccountService::HTTP_METHOD_POST;
  const char realm[] = "My Little Empire";
  const char timestamp[] = "1234567890";
  const char nonce[] = "nonce";

  // oauth_signature generated with
  // http://oauth.googlecode.com/svn/code/javascript/example/signature.html.
  const char expected_header[] =
      "OAuth realm=\"My Little Empire\", "
      "oauth_consumer_key=\"clientkey\", "
      "oauth_nonce=\"nonce\", "
      "oauth_signature=\"jGDhbwQKUvFRDiuol7lYpQ5lNG8%3D\", "
      "oauth_signature_method=\"HMAC-SHA1\", "
      "oauth_timestamp=\"1234567890\", "
      "oauth_token=\"toktok\", "
      "oauth_version=\"1.0\"";
  const std::string signed_header = account.GetSignedAuthHeaderWithTimestamp(
      GURL(base_url), method, realm, timestamp, nonce);

  EXPECT_EQ(expected_header, signed_header);
}

TEST(AccountServiceImplTest, AuthHeaderGeneratedCorrectly2) {
  SuccessfulAccountServiceDelegate delegate("ATTdjN4ih5Cdb7XuNtQLubs71CxCrXGX",
                                            "lqN2Q8hWW7Rg5MmH1ljQDrE7gQyKXygx");
  AccountServiceImpl account("CMghKgBfXSRGppWwrSKEKyaDtn21M8z9",
                             "k8wtriz7QcqJW9XFIH0CguBZz3hGhfyb", &delegate);
  account.LogIn();

  const char base_url[] = "https://link-server.opera.com/pull";
  const AccountService::HttpMethod method = AccountService::HTTP_METHOD_POST;
  const char realm[] = "Opera Link";
  const char timestamp[] = "1359666858";
  const char nonce[] = "RSHyZJsKc5i";

  // oauth_signature generated with
  // http://oauth.googlecode.com/svn/code/javascript/example/signature.html.
  const char expected_header[] =
      "OAuth realm=\"Opera Link\", "
      "oauth_consumer_key=\"CMghKgBfXSRGppWwrSKEKyaDtn21M8z9\", "
      "oauth_nonce=\"RSHyZJsKc5i\", "
      "oauth_signature=\"LB8uq8hNC%2BH20taD%2FhIfEkMIwjE%3D\", "
      "oauth_signature_method=\"HMAC-SHA1\", "
      "oauth_timestamp=\"1359666858\", "
      "oauth_token=\"ATTdjN4ih5Cdb7XuNtQLubs71CxCrXGX\", "
      "oauth_version=\"1.0\"";
  std::string signed_header = account.GetSignedAuthHeaderWithTimestamp(
      GURL(base_url), method, realm, timestamp, nonce);

  EXPECT_EQ(expected_header, signed_header);
}

TEST(AccountServiceImplTest, AuthHeaderGeneratedCorrectlyWithTimeSkew) {
  SuccessfulAccountServiceDelegate delegate("ATTdjN4ih5Cdb7XuNtQLubs71CxCrXGX",
                                            "lqN2Q8hWW7Rg5MmH1ljQDrE7gQyKXygx");
  AccountServiceImpl account("CMghKgBfXSRGppWwrSKEKyaDtn21M8z9",
                             "k8wtriz7QcqJW9XFIH0CguBZz3hGhfyb", &delegate);
  account.LogIn();
  account.SetTimeSkew(259200);

  const char base_url[] = "https://link-server.opera.com/pull";
  const AccountService::HttpMethod method = AccountService::HTTP_METHOD_POST;
  const char realm[] = "Opera Link";
  const char timestamp[] = "1359666858";
  const char nonce[] = "RSHyZJsKc5i";

  // oauth_signature generated with
  // http://oauth.googlecode.com/svn/code/javascript/example/signature.html.
  const char expected_header[] =
      "OAuth realm=\"Opera Link\", "
      "oauth_consumer_key=\"CMghKgBfXSRGppWwrSKEKyaDtn21M8z9\", "
      "oauth_nonce=\"RSHyZJsKc5i\", "
      "oauth_signature=\"wIqMfw8gIIC8XOJ2xHJXVrwz%2F6M%3D\", "
      "oauth_signature_method=\"HMAC-SHA1\", "
      // Timestamp in header matches timestamp + time_skew
      "oauth_timestamp=\"1359926058\", "
      "oauth_token=\"ATTdjN4ih5Cdb7XuNtQLubs71CxCrXGX\", "
      "oauth_version=\"1.0\"";
  std::string signed_header = account.GetSignedAuthHeaderWithTimestamp(
      GURL(base_url), method, realm, timestamp, nonce);

  EXPECT_EQ(expected_header, signed_header);
}

TEST(AccountServiceImplTest, ObserverNotifiedOnLoggedIn) {
  SuccessfulAccountServiceDelegate delegate("a", "z");
  MockAccountObserver observer;
  AccountServiceImpl account("", "", &delegate);
  account.AddObserver(&observer);

  opera_sync::OperaAuthProblem problem;
  problem.set_source(opera_sync::OperaAuthProblem::SOURCE_LOGIN);
  EXPECT_CALL(observer, OnLoginRequested(&account, problem));
  EXPECT_CALL(observer, OnLoggedIn(&account, problem));
  account.LogIn();
}

TEST(AccountServiceImplTest, ObserverNotifiedOnLoginError) {
  const account_util::AuthDataUpdaterError error_code =
      account_util::ADUE_AUTH_ERROR;
  const account_util::AuthOperaComError auth_code =
      account_util::AOCE_420_NOT_AUTHORIZED_REQUEST;
  const char kMessage[] = "you lose";
  opera_sync::OperaAuthProblem problem;
  problem.set_auth_errcode(402);
  problem.set_source(opera_sync::OperaAuthProblem::SOURCE_XMPP);
  problem.set_token("woot");

  FailingAccountServiceDelegate delegate(error_code, auth_code, kMessage);
  MockAccountObserver observer;
  AccountServiceImpl service("", "", &delegate);
  service.AddObserver(&observer);

  EXPECT_CALL(observer, OnLoginRequested(&service, problem));
  EXPECT_CALL(observer,
              OnLoginError(&service, error_code, auth_code, kMessage, problem));
  service.LogIn(problem);
}

TEST(AccountServiceImplTest, ObserverNotifiedOnLoggedOut) {
  SuccessfulAccountServiceDelegate delegate("a", "z");
  MockAccountObserver observer;
  AccountServiceImpl account("", "", &delegate);
  account.AddObserver(&observer);

  account_util::LogoutReason reason = account_util::LR_FOR_TEST_ONLY;

  EXPECT_CALL(observer, OnLoggedOut(&account, reason));
  account.LogOut(account_util::LR_FOR_TEST_ONLY);
}

TEST(AccountServiceImplTest, AuthDataExpired) {
  SuccessfulAccountServiceDelegate delegate("a", "z");
  MockAccountObserver observer;
  opera_sync::OperaAuthProblem problem;
  problem.set_source(opera_sync::OperaAuthProblem::SOURCE_LOGIN);

  AccountServiceImpl account("", "", &delegate);
  account.LogIn();
  account.AddObserver(&observer);

  ASSERT_TRUE(delegate.auth_data_valid());
  ASSERT_FALSE(delegate.logged_out());
  EXPECT_CALL(observer, OnAuthDataExpired(&account, problem));
  account.AuthDataExpired(problem);

  EXPECT_FALSE(delegate.auth_data_valid());
  EXPECT_NE(AccountAuthData(), delegate.auth_data());
  EXPECT_FALSE(delegate.logged_out());
}

TEST(AccountServiceImplTest, LogOut) {
  SuccessfulAccountServiceDelegate delegate("a", "z");

  AccountServiceImpl account("", "", &delegate);
  account.LogIn();

  ASSERT_TRUE(delegate.auth_data_valid());
  ASSERT_FALSE(delegate.logged_out());
  account.LogOut(account_util::LR_FOR_TEST_ONLY);

  EXPECT_TRUE(delegate.auth_data_valid());
  EXPECT_TRUE(delegate.logged_out());
}

}  // namespace
}  // namespace opera
