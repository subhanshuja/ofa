// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/account/oauth_token_fetcher_impl.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "common/account/account_auth_data.h"
#include "common/account/account_util.h"

namespace opera {
namespace {

using ::testing::_;
using ::testing::Return;

const std::string kTestClientKey = "testclient";

class MockURLFetcherFactory : public net::ScopedURLFetcherFactory,
                              public net::URLFetcherFactory {
 public:
  MockURLFetcherFactory() : net::ScopedURLFetcherFactory(this) {}
  virtual ~MockURLFetcherFactory() {}

  MOCK_METHOD4(CreateURLFetcherMock,
               net::URLFetcher*(int id,
                                const GURL& url,
                                net::URLFetcher::RequestType request_type,
                                net::URLFetcherDelegate* d));

  std::unique_ptr<net::URLFetcher> CreateURLFetcher(
      int id,
      const GURL& url,
      net::URLFetcher::RequestType request_type,
      net::URLFetcherDelegate* d) override {
    return std::unique_ptr<net::URLFetcher>(
        CreateURLFetcherMock(id, url, request_type, d));
  }
};

class OAuthTokenFetcherTest : public ::testing::Test {
 public:
  OAuthTokenFetcherTest()
      : error_code_(account_util::ADUE_NO_ERROR),
        auth_code_(account_util::AOCE_NO_ERROR),
        io_thread_("IO thread") {
    base::Thread::Options options;
    options.message_loop_type = base::MessageLoop::TYPE_IO;
    io_thread_.StartWithOptions(options);

    context_getter_ =
        new net::TestURLRequestContextGetter(io_thread_.task_runner());

    token_fetcher_.reset(new OAuthTokenFetcherImpl("", kTestClientKey.c_str(),
                                                   context_getter_.get()));
  }
  ~OAuthTokenFetcherTest() override {}

  net::TestURLFetcher* PrepareTestURLFetcher(int response_code,
                                             const std::string& response_body) {
    net::TestURLFetcher* url_fetcher = new net::TestURLFetcher(
        0, GURL("http://example.com/"), token_fetcher_.get());
    url_fetcher->set_response_code(response_code);
    url_fetcher->SetResponseString(response_body);

    EXPECT_CALL(url_fetcher_factory_, CreateURLFetcherMock(_, _, _, _))
        .WillOnce(Return(url_fetcher));
    return url_fetcher;
  }

  void RequestAuthData() {
    opera_sync::OperaAuthProblem problem;
    token_fetcher_->RequestAuthData(
        problem, base::Bind(&OAuthTokenFetcherTest::OnAuthDataFetched,
                            base::Unretained(this)),
        base::Bind(&OAuthTokenFetcherTest::OnAuthDataFetchFailed,
                   base::Unretained(this)));
  }

 protected:
  base::MessageLoop loop_;
  scoped_refptr<net::TestURLRequestContextGetter> context_getter_;
  MockURLFetcherFactory url_fetcher_factory_;
  std::unique_ptr<OAuthTokenFetcherImpl> token_fetcher_;

  AccountAuthData auth_data_;
  account_util::AuthDataUpdaterError error_code_;
  account_util::AuthOperaComError auth_code_;
  std::string message_;
  opera_sync::OperaAuthProblem problem_;
  base::Thread io_thread_;

 private:
  void OnAuthDataFetched(const AccountAuthData& auth_data,
                         opera_sync::OperaAuthProblem problem) {
    auth_data_ = auth_data;
    error_code_ = account_util::ADUE_NO_ERROR;
    auth_code_ = account_util::AOCE_NO_ERROR;
    problem_ = problem;
    message_.clear();
  }
  void OnAuthDataFetchFailed(account_util::AuthDataUpdaterError error_code,
                             account_util::AuthOperaComError auth_code,
                             const std::string& message,
                             opera_sync::OperaAuthProblem problem) {
    auth_data_ = AccountAuthData();
    error_code_ = error_code;
    auth_code_ = auth_code;
    message_ = message;
    problem_ = problem;
  }
};

TEST_F(OAuthTokenFetcherTest, UploadDataBuiltCorrectly) {
  const struct {
    const char* test_name;
    const char* username;
    const char* password;
    const char* expected_upload_data;  // without the x_consumer_key parameter
  } test_data[] = {
      {"simple", "unreserved", "characters",
       "x_username=unreserved&x_password=characters"},
      {"reserved", "r e  served%", "(char%ct%%s]",
       "x_username=r%20e%20%20served%25&x_password=(char%25ct%25%25s%5D"},
      {"non-ASCII", "arturo", "Blåbærsyltetøy",
       "x_username=arturo&x_password=Bl%C3%A5b%C3%A6rsyltet%C3%B8y"},
  };

  for (unsigned i = 0; i < arraysize(test_data); ++i) {
    SCOPED_TRACE(test_data[i].test_name);

    net::TestURLFetcher* url_fetcher =
        PrepareTestURLFetcher(net::HTTP_OK, "whatever");
    token_fetcher_->SetUserCredentials(test_data[i].username,
                                       test_data[i].password);
    RequestAuthData();
    EXPECT_EQ(std::string(test_data[i].expected_upload_data) +
                  "&x_consumer_key=" + kTestClientKey,
              url_fetcher->upload_data());

    // Must call OnURLFetchComplete() to be able to call RequestAuthData().
    token_fetcher_->OnURLFetchComplete(url_fetcher);
  }
}

TEST_F(OAuthTokenFetcherTest, SuccessResponseParsedCorrectly) {
  const struct {
    const char* test_name;
    const char* response;
    const char* expected_token;
    const char* expected_secret;
  } test_data[] = {
      {"simple", "oauth_token=TOKEN&oauth_token_secret=SECRET", "TOKEN",
       "SECRET"},
      {"reversed", "oauth_token_secret=SECRET&oauth_token=TOKEN", "TOKEN",
       "SECRET"},
      {"redundant",
       "oauth_token_secret=SECRET&redundant=&oauth_token=TOKEN&know_it=not",
       "TOKEN", "SECRET"},
      {"reserved",
       // Note that both "%20" and "+" stand for a space.
       "oauth_token=r%20e%20%20served+%25&"
       "oauth_token_secret=(char%25ct%25%25s%5D",
       "r e  served %", "(char%ct%%s]"},
      {"non-ASCII",
       "oauth_token=arturo&oauth_token_secret=Bl%C3%A5b%C3%A6rsyltet%C3%B8y",
       "arturo", "Blåbærsyltetøy"},
  };

  for (unsigned i = 0; i < arraysize(test_data); ++i) {
    SCOPED_TRACE(test_data[i].test_name);
    net::URLFetcher* url_fetcher =
        PrepareTestURLFetcher(net::HTTP_OK, test_data[i].response);

    RequestAuthData();
    token_fetcher_->OnURLFetchComplete(url_fetcher);

    EXPECT_EQ(test_data[i].expected_token, auth_data_.token);
    EXPECT_EQ(test_data[i].expected_secret, auth_data_.token_secret);
    EXPECT_EQ(account_util::ADUE_NO_ERROR, error_code_);
    EXPECT_EQ(account_util::AOCE_NO_ERROR, auth_code_);
    EXPECT_TRUE(message_.empty());
  }
}

TEST_F(OAuthTokenFetcherTest, ErrorResponseParsedCorrectly) {
  const struct {
    const char* test_name;
    const char* response;
    const char* expected_message;
    account_util::AuthDataUpdaterError error_code;
    account_util::AuthOperaComError auth_code;
  } test_data[] = {
      {"simple", "error=ERROR&success=0", "ERROR",
       account_util::ADUE_AUTH_ERROR,
       account_util::AOCE_AUTH_ERROR_SIMPLE_REQUEST},
      {"reversed", "success=0&error=AnotherERROR", "AnotherERROR",
       account_util::ADUE_AUTH_ERROR,
       account_util::AOCE_AUTH_ERROR_SIMPLE_REQUEST},
      {"reserved",
       // Note that both "%20" and "+" stand for a space.
       "error=User+authentication%20failed&success=0",
       "User authentication failed", account_util::ADUE_AUTH_ERROR,
       account_util::AOCE_AUTH_ERROR_SIMPLE_REQUEST},
      {"non-ASCII and redundant",
       "left&error=Bl%C3%A5b%C3%A6rsyltet%C3%B8y&right=richt", "Blåbærsyltetøy",
       account_util::ADUE_AUTH_ERROR,
       account_util::AOCE_AUTH_ERROR_SIMPLE_REQUEST},
      {"no token", "oauth_token_secret=SECRET", "Cannot parse server response.",
       account_util::ADUE_PARSE_ERROR, account_util::AOCE_NO_ERROR
      },
      {"no secret", "oauth_token=TOKEN&oauth_blabla=bleh",
       "Cannot parse server response.", account_util::ADUE_PARSE_ERROR,
       account_util::AOCE_NO_ERROR},
      {"noise", "pojpoj$POJPokPO$K_30k-30dk,&m0kf3f39fj3mkp__- _-",
       "Cannot parse server response.", account_util::ADUE_PARSE_ERROR,
       account_util::AOCE_NO_ERROR},
      {"chimera 1", "oauth_token=TOKEN&error=Failed&oauth_token_secret=SECRET",
       "Cannot parse server response.", account_util::ADUE_PARSE_ERROR,
       account_util::AOCE_NO_ERROR},
      {"chimera 2", "oauth_token=TOKEN&oauth_token_secret=SECRET&error=Failed",
       "Cannot parse server response.", account_util::ADUE_PARSE_ERROR,
       account_util::AOCE_NO_ERROR},
  };

  for (unsigned i = 0; i < arraysize(test_data); ++i) {
    SCOPED_TRACE(test_data[i].test_name);
    net::URLFetcher* url_fetcher =
        PrepareTestURLFetcher(net::HTTP_OK, test_data[i].response);

    RequestAuthData();
    token_fetcher_->OnURLFetchComplete(url_fetcher);

    EXPECT_TRUE(auth_data_.token.empty());
    EXPECT_TRUE(auth_data_.token_secret.empty());
    EXPECT_EQ(test_data[i].error_code, error_code_);
    EXPECT_EQ(test_data[i].auth_code, auth_code_);
    EXPECT_EQ(test_data[i].expected_message, message_);
  }
}

TEST_F(OAuthTokenFetcherTest, ErrorResponseCodeHandledAsFailure) {
  const int codes[] = {400, 401, 500, 503};
  const std::string messages[] = {"400 (Bad Request)", "401 (Unauthorized)",
                                  "500 (Internal Server Error)",
                                  "503 (Service Unavailable)"};
  static_assert(
      (sizeof(codes) / sizeof(int)) == (sizeof(messages) / sizeof(std::string)),
      "Array size mismatch.");

  for (unsigned i = 0; i < arraysize(codes); ++i) {
    SCOPED_TRACE(base::IntToString(codes[i]));

    net::URLFetcher* url_fetcher = PrepareTestURLFetcher(codes[i], "");

    RequestAuthData();
    token_fetcher_->OnURLFetchComplete(url_fetcher);

    EXPECT_TRUE(auth_data_.token.empty());
    EXPECT_TRUE(auth_data_.token_secret.empty());
    EXPECT_EQ(account_util::ADUE_HTTP_ERROR, error_code_);
    EXPECT_EQ(account_util::AOCE_NO_ERROR, auth_code_);
    EXPECT_EQ(messages[i], message_);
  }
}

TEST_F(OAuthTokenFetcherTest, ErrorRequestStatusHandledAsFailure) {
  const struct {
    net::URLRequestStatus::Status status;
    int error;
    account_util::AuthDataUpdaterError error_code;
    account_util::AuthOperaComError auth_code;
    std::string message;
  } test_data[] = {
      {net::URLRequestStatus::CANCELED, net::ERR_ABORTED,
       account_util::ADUE_NETWORK_ERROR, account_util::AOCE_NO_ERROR,
       "-3 (net::ERR_ABORTED)"},
      {net::URLRequestStatus::CANCELED, net::ERR_FAILED,
       account_util::ADUE_NETWORK_ERROR, account_util::AOCE_NO_ERROR,
       "-2 (net::ERR_FAILED)"},
      {net::URLRequestStatus::FAILED, net::ERR_FAILED,
       account_util::ADUE_NETWORK_ERROR, account_util::AOCE_NO_ERROR,
       "-2 (net::ERR_FAILED)"},
      {net::URLRequestStatus::FAILED, net::ERR_ACCESS_DENIED,
       account_util::ADUE_NETWORK_ERROR, account_util::AOCE_NO_ERROR,
       "-10 (net::ERR_ACCESS_DENIED)"},
  };

  for (unsigned i = 0; i < arraysize(test_data); ++i) {
    SCOPED_TRACE(base::UintToString(i));

    net::TestURLFetcher* url_fetcher = PrepareTestURLFetcher(net::HTTP_OK, "");
    url_fetcher->set_status(
        net::URLRequestStatus(test_data[i].status, test_data[i].error));

    RequestAuthData();
    token_fetcher_->OnURLFetchComplete(url_fetcher);

    EXPECT_TRUE(auth_data_.token.empty());
    EXPECT_TRUE(auth_data_.token_secret.empty());
    EXPECT_EQ(test_data[i].error_code, error_code_);
    EXPECT_EQ(test_data[i].auth_code, auth_code_);
    EXPECT_EQ(test_data[i].message, message_);
  }
}

}  // namespace
}  // namespace opera
