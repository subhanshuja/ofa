// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2017 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#include <algorithm>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "net/base/net_errors.h"
#include "net/base/url_util.h"
#include "net/http/http_status_code.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_status.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "common/auth/auth_service_constants.h"
#include "common/auth/auth_token_consumer.h"
#include "common/auth/auth_token_fetcher.h"

using ::testing::IsNull;
using ::testing::NotNull;
using ::testing::_;

namespace {

const char kHttp200Status[] = "HTTP/1.0 200 OK";

bool GetValueForKey(const std::string& encoded_string,
                    const std::string& key,
                    std::string* value) {
  GURL url("http://example.com/?" + encoded_string);
  return net::GetValueForKeyInQuery(url, key, value);
}

void VerifyPostParam(const std::string& encoded_string,
                     const std::string& key,
                     const std::string& expected_value) {
  std::string decoded_value;
  EXPECT_TRUE(GetValueForKey(encoded_string, key, &decoded_value));
  EXPECT_EQ(decoded_value, expected_value);
}

}  // namespace

namespace opera {

class MockAuthTokenConsumer : public AuthTokenConsumer {
 public:
  MockAuthTokenConsumer() : weak_ptr_factory_(this) {}
  ~MockAuthTokenConsumer() override {}

  // Workaround for inability to mock methods with non-copyable arguments
  void OnAuthSuccess(const std::string& auth_token,
                     std::unique_ptr<UserData> user_data) override {
    OnAuthSuccess_(auth_token, *user_data.get());
  }

  MOCK_METHOD2(OnAuthSuccess_, void(const std::string&, const UserData&));
  MOCK_METHOD2(OnAuthError, void(AuthServiceError, const std::string&));

  base::WeakPtr<AuthTokenConsumer> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  base::WeakPtrFactory<AuthTokenConsumer> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(MockAuthTokenConsumer);
};

class AuthTokenFetcherTest : public testing::Test {
 public:
  AuthTokenFetcherTest() {}
  ~AuthTokenFetcherTest() override {}

  void ReplyWithCSRFToken(const std::string& csrf_token,
                          net::TestURLFetcher* url_fetcher);
  void ReplyWith200OK(net::TestURLFetcher* url_fetcher);
  void ReplyWithAuthToken(const std::string& csrf_token,
                          const std::string& auth_token,
                          net::TestURLFetcher* url_fetcher);

  void SetResponseHeaders(
      net::TestURLFetcher* url_fetcher,
      std::initializer_list<std::pair<const std::string, std::string>> headers);

  void SetUserData(int user_id,
                   const std::string& username,
                   const std::string& password,
                   const std::string& email);

 protected:
  struct TestUserData {
    int user_id;
    std::string username;
    std::string password;
    std::string email;
  };

  net::TestURLRequestContextGetter* GetRequestContext() {
    if (!request_context_getter_.get()) {
      request_context_getter_ =
          new net::TestURLRequestContextGetter(message_loop_.task_runner());
    }
    return request_context_getter_.get();
  }

  TestUserData test_user_data_;
  base::MessageLoopForIO message_loop_;
  net::TestURLFetcherFactory url_fetcher_factory_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_getter_;
};

void AuthTokenFetcherTest::SetResponseHeaders(
    net::TestURLFetcher* url_fetcher,
    std::initializer_list<std::pair<const std::string, std::string>>
        headers_init) {
  ASSERT_THAT(url_fetcher, NotNull());
  scoped_refptr<net::HttpResponseHeaders> response_headers =
      new net::HttpResponseHeaders(kHttp200Status);
  std::for_each(
      std::begin(headers_init), std::end(headers_init),
      [response_headers](const std::pair<std::string, std::string>& h) {
        response_headers->AddHeader(h.first + std::string(": ") + h.second);
      });

  url_fetcher->set_response_headers(response_headers);
}

void AuthTokenFetcherTest::SetUserData(int user_id,
                                       const std::string& username,
                                       const std::string& password,
                                       const std::string& email) {
  test_user_data_.user_id = user_id;
  test_user_data_.username = username;
  test_user_data_.password = password;
  test_user_data_.email = email;
}

void AuthTokenFetcherTest::ReplyWithCSRFToken(
    const std::string& csrf_token,
    net::TestURLFetcher* url_fetcher) {
  ASSERT_THAT(url_fetcher, NotNull());
  net::HttpRequestHeaders request_headers;
  url_fetcher->GetExtraRequestHeaders(&request_headers);
  std::string mobile_client;
  EXPECT_TRUE(request_headers.GetHeader(kMobileClientHeader, &mobile_client));
  EXPECT_EQ(mobile_client, "ofa");

  SetResponseHeaders(url_fetcher, {{kCSRFTokenHeader, csrf_token}});
  ReplyWith200OK(url_fetcher);
}

void AuthTokenFetcherTest::ReplyWith200OK(net::TestURLFetcher* url_fetcher) {
  ASSERT_THAT(url_fetcher, NotNull());
  url_fetcher->set_status(
      net::URLRequestStatus(net::URLRequestStatus::SUCCESS, net::OK));
  url_fetcher->set_response_code(net::HTTP_OK);
  url_fetcher->delegate()->OnURLFetchComplete(url_fetcher);
}

void AuthTokenFetcherTest::ReplyWithAuthToken(
    const std::string& expected_csrf_token,
    const std::string& auth_token,
    net::TestURLFetcher* url_fetcher) {
  ASSERT_THAT(url_fetcher, NotNull());
  net::HttpRequestHeaders request_headers;
  url_fetcher->GetExtraRequestHeaders(&request_headers);
  std::string mobile_client;
  EXPECT_TRUE(request_headers.GetHeader(kMobileClientHeader, &mobile_client));
  EXPECT_EQ(mobile_client, "ofa");

  const std::string upload_data = url_fetcher->upload_data();
  std::string username_or_email;
  EXPECT_TRUE(GetValueForKey(upload_data, "email", &username_or_email));
  EXPECT_TRUE(username_or_email == test_user_data_.username ||
              username_or_email == test_user_data_.email);
  VerifyPostParam(upload_data, "password", test_user_data_.password);
  VerifyPostParam(upload_data, "csrf_token", expected_csrf_token);

  SetResponseHeaders(
      url_fetcher,
      {
          {kUserIDHeader, base::IntToString(test_user_data_.user_id)},
          {kAuthTokenHeader, auth_token},
          {kUserEmailHeader, test_user_data_.email},
          {kUserNameHeader, test_user_data_.username},
          {kEmailVerifiedHeader, "1"},
      });
  ReplyWith200OK(url_fetcher);
}

TEST_F(AuthTokenFetcherTest, AuthenticationSuccess) {
  SetUserData(1337, "cool_username", "cool_password", "cool_guy@example.com");
  const char kCSRFToken[] = "csrf_token";
  const char kAuthToken[] = "auth_token";

  AuthTokenConsumer::UserData reference_data(
      test_user_data_.username, test_user_data_.email, test_user_data_.user_id);
  MockAuthTokenConsumer consumer;
  EXPECT_CALL(consumer, OnAuthSuccess_(kAuthToken, reference_data)).Times(1);
  EXPECT_CALL(consumer, OnAuthError(_, _)).Times(0);
  AuthTokenFetcher token_fetcher(GetRequestContext());
  token_fetcher.RequestAuthTokenForOAuth2(test_user_data_.username,
                                          test_user_data_.password,
                                          consumer.GetWeakPtr());

  net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(0);
  ReplyWithCSRFToken(kCSRFToken, fetcher);

  fetcher = url_fetcher_factory_.GetFetcherByID(1);
  ReplyWithAuthToken(kCSRFToken, kAuthToken, fetcher);
}

TEST_F(AuthTokenFetcherTest, DropsRequestOnConsumerLoss) {
  AuthTokenFetcher token_fetcher(GetRequestContext());

  {
    auto consumer = base::MakeUnique<MockAuthTokenConsumer>();
    EXPECT_CALL(*consumer, OnAuthSuccess_(_, _)).Times(0);
    EXPECT_CALL(*consumer, OnAuthError(_, _)).Times(0);

    token_fetcher.RequestAuthTokenForOAuth2("foo", "bar",
                                            consumer->GetWeakPtr());
  }

  ReplyWithCSRFToken("csrf_token", url_fetcher_factory_.GetFetcherByID(0));

  {
    auto consumer = base::MakeUnique<MockAuthTokenConsumer>();
    EXPECT_CALL(*consumer, OnAuthSuccess_(_, _)).Times(0);
    EXPECT_CALL(*consumer, OnAuthError(_, _)).Times(0);

    token_fetcher.RequestAuthTokenForOAuth2("foo", "bar",
                                            consumer->GetWeakPtr());
    ReplyWithCSRFToken("csrf_token", url_fetcher_factory_.GetFetcherByID(1));
  }

  ReplyWith200OK(url_fetcher_factory_.GetFetcherByID(2));
}

TEST_F(AuthTokenFetcherTest, FailWhen200MissingHeaders) {
  MockAuthTokenConsumer consumer;
  EXPECT_CALL(consumer, OnAuthSuccess_(_, _)).Times(0);
  EXPECT_CALL(consumer, OnAuthError(UNEXPECTED_RESPONSE, _)).Times(1);

  AuthTokenFetcher token_fetcher(GetRequestContext());
  token_fetcher.RequestAuthTokenForOAuth2("foo", "bar", consumer.GetWeakPtr());
  net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(0);
  SetResponseHeaders(fetcher, {{}});
  ReplyWith200OK(fetcher);
}

// TODO(ingemara): AUTH-970
TEST_F(AuthTokenFetcherTest, AssumeIncompleteHeadersInvalidCredentials) {
  MockAuthTokenConsumer consumer;
  EXPECT_CALL(consumer, OnAuthSuccess_(_, _)).Times(0);
  EXPECT_CALL(consumer, OnAuthError(INVALID_CREDENTIALS, _)).Times(1);

  AuthTokenFetcher token_fetcher(GetRequestContext());
  token_fetcher.RequestAuthTokenForOAuth2("foo", "bar", consumer.GetWeakPtr());
  ReplyWithCSRFToken("csrf", url_fetcher_factory_.GetFetcherByID(0));

  net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(1);
  ASSERT_THAT(fetcher, NotNull());

  SetResponseHeaders(fetcher, {{kAuthTokenHeader, "invalid_token"}});
  ReplyWith200OK(fetcher);
}

TEST_F(AuthTokenFetcherTest, FailForConnectionTimeout) {
  MockAuthTokenConsumer consumer;
  EXPECT_CALL(consumer, OnAuthSuccess_(_, _)).Times(0);
  EXPECT_CALL(consumer, OnAuthError(CONNECTION_FAILED, _)).Times(1);

  AuthTokenFetcher token_fetcher(GetRequestContext());
  token_fetcher.RequestAuthTokenForOAuth2("foo", "bar", consumer.GetWeakPtr());

  net::TestURLFetcher* fetcher = url_fetcher_factory_.GetFetcherByID(0);
  ASSERT_THAT(fetcher, NotNull());
  net::URLRequestStatus status(net::URLRequestStatus::FAILED,
                               net::ERR_TIMED_OUT);
  fetcher->set_status(status);
  fetcher->delegate()->OnURLFetchComplete(fetcher);
}

}  // namespace opera
