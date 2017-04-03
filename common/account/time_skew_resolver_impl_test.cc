// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include <string>
#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_context_getter.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "common/account/time_skew_resolver_impl.h"

namespace opera {

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace {

class SingleThreadTaskRunnerMock : public base::SingleThreadTaskRunner {
 public:
  bool RunsTasksOnCurrentThread() const override { return true; }
  bool PostNonNestableDelayedTask(const tracked_objects::Location& from_here,
                                  const base::Closure& task,
                                  base::TimeDelta delay) override {
    return true;
  }
  bool PostDelayedTask(const tracked_objects::Location& from_here,
                       const base::Closure& task,
                       base::TimeDelta delay) override {
    return true;
  }

 private:
  ~SingleThreadTaskRunnerMock() override {}
};

class URLRequestContextGetterMock : public net::URLRequestContextGetter {
 public:
  MOCK_METHOD0(GetURLRequestContext, net::URLRequestContext*());
  MOCK_CONST_METHOD0(GetNetworkTaskRunner,
                     scoped_refptr<base::SingleThreadTaskRunner>());

 private:
  ~URLRequestContextGetterMock() {}
};

class URLResultHolder : public net::TestURLFetcher {
 public:
  explicit URLResultHolder(const std::string& response, bool return_value)
      : net::TestURLFetcher(0, GURL("http://mockurl.com"), nullptr),
        response_(response),
        return_value_(return_value) {}

  int GetResponseCode() const override { return 0; }

  bool GetResponseAsString(std::string* out) const override {
    *out = response_;
    return return_value_;
  }

 private:
  const std::string response_;
  const bool return_value_;
};

}  // anonymous namespace

class TimeSkewResolverImplTest : public ::testing::Test,
                                 public net::TestURLFetcherDelegateForTests {
 public:
  TimeSkewResolverImplTest()
      : task_runner_(new SingleThreadTaskRunnerMock),
        url_request_context_getter_mock_(new URLRequestContextGetterMock),
        url_request_context_getter_(url_request_context_getter_mock_),
        test_url_fetcher_factory_() {
    test_url_fetcher_factory_.SetDelegateForTests(this);

    ON_CALL(*url_request_context_getter_mock_, GetNetworkTaskRunner())
        .WillByDefault(Return(task_runner_));
  }

  void SetUp() override {
    time_skew_resolver_.reset(new TimeSkewResolverImpl(
        base::Bind(&net::TestURLFetcherFactory::CreateURLFetcher,
                   base::Unretained(&test_url_fetcher_factory_)),
        url_request_context_getter_.get(), GURL("https://auth.opera.com/")));

    was_callback_run_ = false;
  }

  void TearDown() override {
    time_skew_resolver_.reset();
    result_.reset();
  }

  void SetServerResponse(std::string response, bool fetcher_return_value) {
    response_ = response;
    fetcher_return_value_ = fetcher_return_value;
  }

  void OnRequestStart(int fetcher_id) override {
    net::TestURLFetcher* url_fetcher =
        test_url_fetcher_factory_.GetFetcherByID(fetcher_id);

    url_fetcher->delegate()->OnURLFetchComplete(
        new URLResultHolder(response_, fetcher_return_value_));
  }
  void OnChunkUpload(int fetcher_id) override {}
  void OnRequestEnd(int fetcher_id) override {}

  TimeSkewResolver::ResultCallback GetCallback() {
    return base::Bind(&TimeSkewResolverImplTest::TestCallback,
                      base::Unretained(this));
  }

 protected:
  std::unique_ptr<TimeSkewResolver> time_skew_resolver_;
  bool was_callback_run_;
  std::unique_ptr<TimeSkewResolver::ResultValue> result_;

 private:
  void TestCallback(const TimeSkewResolver::ResultValue& result) {
    was_callback_run_ = true;
    result_.reset(new TimeSkewResolver::ResultValue(result));
  }

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  URLRequestContextGetterMock* url_request_context_getter_mock_;
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;
  net::TestURLFetcherFactory test_url_fetcher_factory_;
  std::string response_;
  bool fetcher_return_value_;
};

TEST_F(TimeSkewResolverImplTest, HandlesOKResponse) {
  SetServerResponse(
      "{\"status\":\"success\",\"diff\":\"NaN\",\"message\":\"\",\"code\":200}",
      true);

  time_skew_resolver_->RequestTimeSkew(GetCallback());

  EXPECT_TRUE(was_callback_run_);
  EXPECT_EQ(TimeSkewResolver::QueryResult::TIME_OK, result_->result);
  EXPECT_EQ("", result_->error_message);
}

TEST_F(TimeSkewResolverImplTest, HandlesTimeSkewResponse) {
  SetServerResponse(
      "{\"status\":\"error\",\"diff\":12345,"
      "\"message\":\"Clock differs too much\",\"code\":401}",
      true);

  time_skew_resolver_->RequestTimeSkew(GetCallback());

  EXPECT_TRUE(was_callback_run_);
  EXPECT_EQ(TimeSkewResolver::QueryResult::TIME_SKEW, result_->result);
  EXPECT_EQ(12345, result_->time_skew);
  EXPECT_EQ("Clock differs too much", result_->error_message);
}

TEST_F(TimeSkewResolverImplTest, HandlesBadRequestResponse) {
  SetServerResponse(
      "{\"status\":\"error\",\"diff\":\"NaN\","
      "\"message\":\"Bad request\",\"code\":400}",
      true);

  time_skew_resolver_->RequestTimeSkew(GetCallback());

  EXPECT_TRUE(was_callback_run_);
  EXPECT_EQ(TimeSkewResolver::QueryResult::BAD_REQUEST_ERROR, result_->result);
  EXPECT_EQ("Bad request", result_->error_message);
}

TEST_F(TimeSkewResolverImplTest, HandlesBadJSONResponse) {
  SetServerResponse("not valid JSON", true);

  time_skew_resolver_->RequestTimeSkew(GetCallback());

  EXPECT_TRUE(was_callback_run_);
  EXPECT_EQ(TimeSkewResolver::QueryResult::INVALID_RESPONSE_ERROR,
            result_->result);
}

TEST_F(TimeSkewResolverImplTest, HandlesNoResponse) {
  SetServerResponse("whatever message", false);

  time_skew_resolver_->RequestTimeSkew(GetCallback());

  EXPECT_TRUE(was_callback_run_);
  EXPECT_EQ(TimeSkewResolver::QueryResult::INVALID_RESPONSE_ERROR,
            result_->result);
}

}  // namespace opera
