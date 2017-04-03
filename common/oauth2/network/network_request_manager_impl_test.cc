// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/network/network_request_manager_impl.h"

#include <string>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/simple_test_clock.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/test/test_mock_time_task_runner.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "common/oauth2/network/network_request_mock.h"
#include "common/oauth2/util/constants.h"

namespace opera {
namespace oauth2 {
namespace {

net::BackoffEntry::Policy kTestingBackoffPolicy = {
    // Number of initial errors (in sequence) to ignore before applying
    // exponential back-off rules.
    0,

    // Initial delay for exponential back-off in ms.
    1000,  // 1 second.

    // Factor by which the waiting time will be multiplied.
    2,

    // Fuzzing percentage. ex: 10% will spread requests randomly
    // between 90%-100% of the calculated time.
    0,  // 0%.

    // Maximum amount of time we are willing to delay our request in ms.
    5 * 60 * 1000,  // 5 minutes.

    // Time to keep an entry from being discarded even when it
    // has no significant state, -1 to never discard.
    -1,

    // Don't use initial delay unless the last request was an error.
    false,
};

const GURL kMockOAuth1BaseURL("https://mock.oauth1.base.url/");
const GURL kMockOAuth2BaseURL("https://mock.oauth2.base.url/");
const GURL kMockOAuth1BaseURLInsecure("http://mock.oauth1.base.url/");
const std::string kMockRequestPath("/mocked/request/path/");

const std::string kThrottledBody =
    "{\"detail\":\"Request was throttled.Expected available in 86400.0 "
    "seconds.\"}";

using ::testing::_;
using ::testing::Return;

class MockRequestManagerConsumer : public NetworkRequestManager::Consumer {
 public:
  MockRequestManagerConsumer() : weak_ptr_factory_(this) {}
  ~MockRequestManagerConsumer() override {}

  MOCK_METHOD2(OnNetworkRequestFinished,
               void(scoped_refptr<NetworkRequest>, NetworkResponseStatus));
  MOCK_CONST_METHOD0(GetConsumerName, std::string());

  base::WeakPtrFactory<MockRequestManagerConsumer> weak_ptr_factory_;
};

class NetworkRequestManagerImplTest : public ::testing::Test {
 public:
  NetworkRequestManagerImplTest()
      : request_context_getter_(
            new net::TestURLRequestContextGetter(message_loop_.task_runner())),
        fetcher_factory_(
            nullptr,
            base::Bind(&NetworkRequestManagerImplTest::CreateFetcher,
                       base::Unretained(this))),
        fetcher_(nullptr) {
    test_task_runner_ = new base::TestMockTimeTaskRunner;
    response_headers_ = new net::HttpResponseHeaders("");

    NetworkRequestManagerImpl::RequestUrlsConfig url_config;
    url_config[RMUT_OAUTH1] = std::make_pair(kMockOAuth1BaseURL, false);
    url_config[RMUT_OAUTH2] = std::make_pair(kMockOAuth2BaseURL, false);
    CreateRequestManager(url_config);
  }
  ~NetworkRequestManagerImplTest() override {}

  void CreateRequestManager(
      NetworkRequestManagerImpl::RequestUrlsConfig url_config) {
    test_clock_ = test_task_runner_->GetMockClock().release();
    request_manager_.reset(new NetworkRequestManagerImpl(
        nullptr, url_config, request_context_getter_.get(),
        kTestingBackoffPolicy, test_task_runner_->GetMockTickClock(),
        base::WrapUnique(test_clock_), test_task_runner_));
  }

  void TearDown() override {
    EXPECT_FALSE(test_task_runner_->HasPendingTask());
  }

  std::unique_ptr<net::FakeURLFetcher> CreateFetcher(
      const GURL& url,
      net::URLFetcherDelegate* delegate,
      const std::string& response_data,
      net::HttpStatusCode response_code,
      net::URLRequestStatus::Status status) {
    std::unique_ptr<net::FakeURLFetcher> fetcher(new net::FakeURLFetcher(
        url, delegate, response_data, response_code, status));
    fetcher->set_response_headers(response_headers_);
    fetcher_ = fetcher.get();
    return fetcher;
  }

 protected:
  base::MessageLoopForIO message_loop_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_getter_;
  net::FakeURLFetcherFactory fetcher_factory_;
  net::TestURLFetcher* fetcher_;

  std::unique_ptr<NetworkRequestManagerImpl> request_manager_;
  scoped_refptr<NetworkRequestMock> mock_request_;
  MockRequestManagerConsumer mock_consumer_;
  base::Clock* test_clock_;
  scoped_refptr<base::TestMockTimeTaskRunner> test_task_runner_;
  scoped_refptr<net::HttpResponseHeaders> response_headers_;
};

}  // namespace

TEST_F(NetworkRequestManagerImplTest, DiagnosticSnapshotContents) {
  mock_request_ = new NetworkRequestMock;
  EXPECT_CALL(*mock_request_, GetPath())
      .WillRepeatedly(Return(kMockRequestPath));
  EXPECT_CALL(*mock_request_, GetContent())
      .WillRepeatedly(Return("mock=request&content=mock"));
  EXPECT_CALL(*mock_request_, GetHTTPRequestType())
      .WillRepeatedly(Return(net::URLFetcher::POST));
  EXPECT_CALL(*mock_request_, GetRequestContentType())
      .WillRepeatedly(Return("mock-content-type"));
  EXPECT_CALL(*mock_request_, GetLoadFlags()).WillRepeatedly(Return(0));
  EXPECT_CALL(*mock_request_,
              TryResponse(net::HTTP_BAD_REQUEST, "mock-response-body"))
      .WillRepeatedly(Return(RS_PARSE_PROBLEM));
  EXPECT_CALL(*mock_request_, GetDiagnosticDescription())
      .WillRepeatedly(Return("Request description"));
  EXPECT_CALL(*mock_request_, GetRequestManagerUrlType())
      .WillRepeatedly(Return(RMUT_OAUTH2));
  EXPECT_CALL(*mock_request_, GetQueryString()).WillRepeatedly(Return(""));
  EXPECT_CALL(*mock_request_, GetExtraRequestHeaders())
      .WillRepeatedly(Return(std::vector<std::string>{}));

  EXPECT_CALL(mock_consumer_, GetConsumerName())
      .WillOnce(Return("Mock consumer"));

  fetcher_factory_.SetFakeResponse(kMockOAuth2BaseURL.Resolve(kMockRequestPath),
                                   "mock-response-body", net::HTTP_BAD_REQUEST,
                                   net::URLRequestStatus::SUCCESS);

  request_manager_->StartRequest(mock_request_,
                                 mock_consumer_.weak_ptr_factory_.GetWeakPtr());
  test_task_runner_->FastForwardUntilNoTasksRemain();

  std::unique_ptr<base::DictionaryValue> detail(new base::DictionaryValue);
  std::unique_ptr<base::ListValue> requests(new base::ListValue);
  std::unique_ptr<base::DictionaryValue> request0(new base::DictionaryValue);
  std::unique_ptr<base::DictionaryValue> base_urls(new base::DictionaryValue);
  base_urls->SetString("OAUTH1", kMockOAuth1BaseURL.spec());
  base_urls->SetString("OAUTH2", kMockOAuth2BaseURL.spec());

  request0->SetString("base_request_url", "OAUTH2");
  request0->SetString("consumer", "Mock consumer");
  request0->SetString("detail", "Request description");
  request0->SetDouble("last_request_time", test_clock_->Now().ToJsTime());
  request0->SetDouble("next_request_time", test_clock_->Now().ToJsTime());
  request0->SetString("path", "/mocked/request/path/");
  request0->SetInteger("request_count", 1);
  request0->SetInteger("type", 1);

  requests->Append(std::move(request0));

  detail->Set("base_request_urls", std::move(base_urls));
  detail->Set("requests", std::move(requests));

  EXPECT_EQ("network_manager", request_manager_->GetDiagnosticName());
  auto actual = request_manager_->GetDiagnosticSnapshot();
  EXPECT_NE(nullptr, actual);
  EXPECT_TRUE(detail->Equals(actual.get())) << "Actual: " << *actual
                                            << "\nExpected: " << *detail;
}

TEST_F(NetworkRequestManagerImplTest, RetryWorks) {
  mock_request_ = new NetworkRequestMock;
  EXPECT_CALL(*mock_request_, GetPath())
      .WillRepeatedly(Return(kMockRequestPath));
  EXPECT_CALL(*mock_request_, GetContent())
      .WillRepeatedly(Return("mock=request&content=mock"));
  EXPECT_CALL(*mock_request_, GetHTTPRequestType())
      .WillRepeatedly(Return(net::URLFetcher::POST));
  EXPECT_CALL(*mock_request_, GetRequestContentType())
      .WillRepeatedly(Return("mock-content-type"));
  EXPECT_CALL(*mock_request_, GetLoadFlags()).WillRepeatedly(Return(0));
  EXPECT_CALL(*mock_request_,
              TryResponse(net::HTTP_BAD_REQUEST, "mock-response-body"))
      .WillOnce(Return(RS_PARSE_PROBLEM));
  EXPECT_CALL(*mock_request_, TryResponse(net::HTTP_OK, "mock-response-body"))
      .WillOnce(Return(RS_OK));
  EXPECT_CALL(*mock_request_, GetDiagnosticDescription())
      .WillRepeatedly(Return("Request description"));
  EXPECT_CALL(*mock_request_, GetRequestManagerUrlType())
      .WillRepeatedly(Return(RMUT_OAUTH2));
  EXPECT_CALL(*mock_request_, GetQueryString()).WillRepeatedly(Return(""));
  EXPECT_CALL(*mock_request_, GetExtraRequestHeaders())
      .WillRepeatedly(Return(std::vector<std::string>{}));

  EXPECT_CALL(mock_consumer_, GetConsumerName())
      .WillRepeatedly(Return("mock_consumer"));

  fetcher_factory_.SetFakeResponse(kMockOAuth2BaseURL.Resolve(kMockRequestPath),
                                   "mock-response-body", net::HTTP_BAD_REQUEST,
                                   net::URLRequestStatus::SUCCESS);

  request_manager_->StartRequest(mock_request_,
                                 mock_consumer_.weak_ptr_factory_.GetWeakPtr());

  // Run until the fetcher is started, no response yet.
  test_task_runner_->FastForwardUntilNoTasksRemain();
  base::Time last_request_time = test_clock_->Now();
  base::Time next_request_time = test_clock_->Now();
  base::Time last_response_time;

  std::unique_ptr<base::DictionaryValue> detail(new base::DictionaryValue);
  std::unique_ptr<base::ListValue> requests(new base::ListValue);
  std::unique_ptr<base::DictionaryValue> request0(new base::DictionaryValue);
  std::unique_ptr<base::DictionaryValue> base_urls(new base::DictionaryValue);
  std::unique_ptr<base::DictionaryValue> response_detail(
      new base::DictionaryValue);

  base_urls->SetString("OAUTH1", kMockOAuth1BaseURL.spec());
  base_urls->SetString("OAUTH2", kMockOAuth2BaseURL.spec());

  request0->SetString("base_request_url", "OAUTH2");
  request0->SetString("consumer", "mock_consumer");
  request0->SetString("detail", "Request description");
  request0->SetDouble("last_request_time", last_request_time.ToJsTime());
  request0->SetDouble("next_request_time", next_request_time.ToJsTime());
  request0->SetString("path", "/mocked/request/path/");
  request0->SetInteger("request_count", 1);
  request0->SetInteger("type", 1);
  requests->Append(std::move(request0));

  detail->Set("requests", std::move(requests));
  detail->Set("base_request_urls", std::move(base_urls));

  EXPECT_EQ("network_manager", request_manager_->GetDiagnosticName());
  auto actual = request_manager_->GetDiagnosticSnapshot();
  EXPECT_NE(nullptr, actual);
  EXPECT_TRUE(detail->Equals(actual.get())) << "Actual: " << *actual
                                            << "\nExpected: " << *detail;

  // Run until the fetcher returns.
  base::RunLoop().RunUntilIdle();
  base::TimeDelta backoff_delay = base::TimeDelta::FromSeconds(1);
  // Response coming now.
  last_response_time = test_clock_->Now();
  // Next request scheduled with backoff.
  next_request_time = last_response_time + backoff_delay;

  // Waiting for retry
  detail.reset(new base::DictionaryValue);
  requests.reset(new base::ListValue);
  request0.reset(new base::DictionaryValue);
  base_urls.reset(new base::DictionaryValue);
  response_detail.reset(new base::DictionaryValue);

  base_urls->SetString("OAUTH1", kMockOAuth1BaseURL.spec());
  base_urls->SetString("OAUTH2", kMockOAuth2BaseURL.spec());

  response_detail->SetInteger("http_status", 400);
  response_detail->SetString("request_status", "PARSE_PROBLEM");
  response_detail->SetDouble("response_time", last_response_time.ToJsTime());
  response_detail->SetString("network_error", "net::OK");

  request0->SetString("base_request_url", "OAUTH2");
  request0->SetString("consumer", "mock_consumer");
  request0->SetString("detail", "Request description");
  request0->SetDouble("last_request_time", last_request_time.ToJsTime());
  request0->SetDouble("next_request_time", next_request_time.ToJsTime());
  request0->SetString("path", "/mocked/request/path/");
  request0->SetInteger("request_count", 1);
  request0->SetInteger("type", 1);
  request0->Set("last_response", std::move(response_detail));
  requests->Append(std::move(request0));

  detail->Set("requests", std::move(requests));
  detail->Set("base_request_urls", std::move(base_urls));

  EXPECT_EQ("network_manager", request_manager_->GetDiagnosticName());
  actual = request_manager_->GetDiagnosticSnapshot();
  EXPECT_NE(nullptr, actual);
  EXPECT_TRUE(detail->Equals(actual.get())) << "Actual: " << *actual
                                            << "\nExpected: " << *detail;

  // Run until the fetcher is started for retry.
  fetcher_factory_.SetFakeResponse(kMockOAuth2BaseURL.Resolve(kMockRequestPath),
                                   "mock-response-body", net::HTTP_OK,
                                   net::URLRequestStatus::SUCCESS);
  EXPECT_CALL(
      mock_consumer_,
      OnNetworkRequestFinished(
          static_cast<scoped_refptr<NetworkRequest>>(mock_request_), RS_OK))
      .Times(1);

  test_task_runner_->FastForwardBy(backoff_delay);
  // Request was scheduled now.
  last_request_time = test_clock_->Now();
  next_request_time = base::Time();

  detail.reset(new base::DictionaryValue);
  requests.reset(new base::ListValue);
  request0.reset(new base::DictionaryValue);
  base_urls.reset(new base::DictionaryValue);
  response_detail.reset(new base::DictionaryValue);

  base_urls->SetString("OAUTH1", kMockOAuth1BaseURL.spec());
  base_urls->SetString("OAUTH2", kMockOAuth2BaseURL.spec());

  response_detail->SetInteger("http_status", 400);
  response_detail->SetString("request_status", "PARSE_PROBLEM");
  response_detail->SetDouble("response_time", last_response_time.ToJsTime());
  response_detail->SetString("network_error", "net::OK");

  request0->SetString("base_request_url", "OAUTH2");
  request0->SetString("consumer", "mock_consumer");
  request0->SetString("detail", "Request description");
  request0->SetDouble("last_request_time", last_request_time.ToJsTime());
  request0->SetDouble("next_request_time", next_request_time.ToJsTime());
  request0->SetString("path", "/mocked/request/path/");
  request0->SetInteger("request_count", 2);
  request0->SetInteger("type", 1);
  request0->Set("last_response", std::move(response_detail));
  requests->Append(std::move(request0));

  detail->Set("requests", std::move(requests));
  detail->Set("base_request_urls", std::move(base_urls));

  EXPECT_EQ("network_manager", request_manager_->GetDiagnosticName());
  actual = request_manager_->GetDiagnosticSnapshot();
  EXPECT_NE(nullptr, actual);
  EXPECT_TRUE(detail->Equals(actual.get())) << "Actual: " << *actual
                                            << "\nExpected: " << *detail;

  // Run until the fetcher returns.
  base::RunLoop().RunUntilIdle();
  // According to policy.
  backoff_delay *= 2;
  // Response coming now.
  last_response_time = test_clock_->Now();
  // Next request scheduled with backoff.
  next_request_time = last_response_time + backoff_delay;

  // Waiting for retry
  detail.reset(new base::DictionaryValue);
  requests.reset(new base::ListValue);
  base_urls.reset(new base::DictionaryValue);

  base_urls->SetString("OAUTH1", kMockOAuth1BaseURL.spec());
  base_urls->SetString("OAUTH2", kMockOAuth2BaseURL.spec());

  detail->Set("requests", std::move(requests));
  detail->Set("base_request_urls", std::move(base_urls));

  EXPECT_EQ("network_manager", request_manager_->GetDiagnosticName());
  actual = request_manager_->GetDiagnosticSnapshot();
  EXPECT_NE(nullptr, actual);
  EXPECT_TRUE(detail->Equals(actual.get())) << "Actual: " << *actual
                                            << "\nExpected: " << *detail;
}

TEST_F(NetworkRequestManagerImplTest, ManagerUsesExtraHeaders) {
  mock_request_ = new NetworkRequestMock;

  EXPECT_CALL(*mock_request_, GetPath())
      .WillRepeatedly(Return(kMockRequestPath));
  EXPECT_CALL(*mock_request_, GetContent())
      .WillRepeatedly(Return("mock=request&content=mock"));
  EXPECT_CALL(*mock_request_, GetHTTPRequestType())
      .WillRepeatedly(Return(net::URLFetcher::POST));
  EXPECT_CALL(*mock_request_, GetRequestContentType())
      .WillRepeatedly(Return("mock-content-type"));
  EXPECT_CALL(*mock_request_, GetLoadFlags()).WillRepeatedly(Return(0));
  EXPECT_CALL(*mock_request_, TryResponse(net::HTTP_OK, "mock-response-body"))
      .WillRepeatedly(Return(RS_OK));
  EXPECT_CALL(*mock_request_, GetRequestManagerUrlType())
      .WillRepeatedly(Return(RMUT_OAUTH2));
  EXPECT_CALL(*mock_request_, GetQueryString()).WillRepeatedly(Return(""));
  EXPECT_CALL(*mock_request_, GetExtraRequestHeaders())
      .WillRepeatedly(Return(std::vector<std::string>{
          "header_name:header_value", "other_header:other_value"}));

  EXPECT_CALL(
      mock_consumer_,
      OnNetworkRequestFinished(
          static_cast<scoped_refptr<NetworkRequest>>(mock_request_), RS_OK))
      .Times(1);

  fetcher_factory_.SetFakeResponse(kMockOAuth2BaseURL.Resolve(kMockRequestPath),
                                   "mock-response-body", net::HTTP_OK,
                                   net::URLRequestStatus::SUCCESS);

  request_manager_->StartRequest(mock_request_,
                                 mock_consumer_.weak_ptr_factory_.GetWeakPtr());
  test_task_runner_->FastForwardUntilNoTasksRemain();

  net::HttpRequestHeaders actual_extra_headers;
  fetcher_->GetExtraRequestHeaders(&actual_extra_headers);

  ASSERT_FALSE(actual_extra_headers.IsEmpty());
  ASSERT_TRUE(actual_extra_headers.HasHeader("header_name"));
  ASSERT_TRUE(actual_extra_headers.HasHeader("other_header"));
  std::string actual_header_value;
  ASSERT_TRUE(
      actual_extra_headers.GetHeader("header_name", &actual_header_value));
  ASSERT_EQ("header_value", actual_header_value);
  ASSERT_TRUE(
      actual_extra_headers.GetHeader("other_header", &actual_header_value));
  ASSERT_EQ("other_value", actual_header_value);

  base::RunLoop().RunUntilIdle();
}

TEST_F(NetworkRequestManagerImplTest, ManagerUsesQueryString) {
  const std::string kQueryString = "key1=value1&key2=value2";
  const GURL kExpectedUrl =
      kMockOAuth2BaseURL.Resolve(kMockRequestPath).Resolve("?" + kQueryString);

  mock_request_ = new NetworkRequestMock;

  EXPECT_CALL(*mock_request_, GetPath())
      .WillRepeatedly(Return(kMockRequestPath));
  EXPECT_CALL(*mock_request_, GetContent())
      .WillRepeatedly(Return("mock=request&content=mock"));
  EXPECT_CALL(*mock_request_, GetHTTPRequestType())
      .WillRepeatedly(Return(net::URLFetcher::POST));
  EXPECT_CALL(*mock_request_, GetRequestContentType())
      .WillRepeatedly(Return("mock-content-type"));
  EXPECT_CALL(*mock_request_, GetLoadFlags()).WillRepeatedly(Return(0));
  EXPECT_CALL(*mock_request_, TryResponse(net::HTTP_OK, "mock-response-body"))
      .WillRepeatedly(Return(RS_OK));
  EXPECT_CALL(*mock_request_, GetRequestManagerUrlType())
      .WillRepeatedly(Return(RMUT_OAUTH2));
  EXPECT_CALL(*mock_request_, GetQueryString())
      .WillRepeatedly(Return(kQueryString));
  EXPECT_CALL(*mock_request_, GetExtraRequestHeaders())
      .WillRepeatedly(Return(std::vector<std::string>{}));

  EXPECT_CALL(
      mock_consumer_,
      OnNetworkRequestFinished(
          static_cast<scoped_refptr<NetworkRequest>>(mock_request_), RS_OK))
      .Times(1);

  fetcher_factory_.SetFakeResponse(kExpectedUrl, "mock-response-body",
                                   net::HTTP_OK,
                                   net::URLRequestStatus::SUCCESS);

  request_manager_->StartRequest(mock_request_,
                                 mock_consumer_.weak_ptr_factory_.GetWeakPtr());
  test_task_runner_->FastForwardUntilNoTasksRemain();

  EXPECT_EQ(kExpectedUrl, fetcher_->GetURL());

  base::RunLoop().RunUntilIdle();
}

TEST_F(NetworkRequestManagerImplTest, SmokeResponseOK) {
  mock_request_ = new NetworkRequestMock;

  EXPECT_CALL(*mock_request_, GetPath())
      .WillRepeatedly(Return(kMockRequestPath));
  EXPECT_CALL(*mock_request_, GetContent())
      .WillRepeatedly(Return("mock=request&content=mock"));
  EXPECT_CALL(*mock_request_, GetHTTPRequestType())
      .WillRepeatedly(Return(net::URLFetcher::POST));
  EXPECT_CALL(*mock_request_, GetRequestContentType())
      .WillRepeatedly(Return("mock-content-type"));
  EXPECT_CALL(*mock_request_, GetLoadFlags()).WillRepeatedly(Return(0));
  EXPECT_CALL(*mock_request_, TryResponse(net::HTTP_OK, "mock-response-body"))
      .WillRepeatedly(Return(RS_OK));
  EXPECT_CALL(*mock_request_, GetRequestManagerUrlType())
      .WillRepeatedly(Return(RMUT_OAUTH2));
  EXPECT_CALL(*mock_request_, GetQueryString()).WillRepeatedly(Return(""));
  EXPECT_CALL(*mock_request_, GetExtraRequestHeaders())
      .WillRepeatedly(Return(std::vector<std::string>{}));

  EXPECT_CALL(
      mock_consumer_,
      OnNetworkRequestFinished(
          static_cast<scoped_refptr<NetworkRequest>>(mock_request_), RS_OK))
      .Times(1);

  fetcher_factory_.SetFakeResponse(kMockOAuth2BaseURL.Resolve(kMockRequestPath),
                                   "mock-response-body", net::HTTP_OK,
                                   net::URLRequestStatus::SUCCESS);

  request_manager_->StartRequest(mock_request_,
                                 mock_consumer_.weak_ptr_factory_.GetWeakPtr());
  test_task_runner_->FastForwardUntilNoTasksRemain();
  base::RunLoop().RunUntilIdle();
}

TEST_F(NetworkRequestManagerImplTest, SmokeResponseHTTPProblem) {
  mock_request_ = new NetworkRequestMock;

  EXPECT_CALL(*mock_request_, GetPath())
      .WillRepeatedly(Return(kMockRequestPath));
  EXPECT_CALL(*mock_request_, GetContent())
      .WillRepeatedly(Return("mock=request&content=mock"));
  EXPECT_CALL(*mock_request_, GetHTTPRequestType())
      .WillRepeatedly(Return(net::URLFetcher::POST));
  EXPECT_CALL(*mock_request_, GetRequestContentType())
      .WillRepeatedly(Return("mock-content-type"));
  EXPECT_CALL(*mock_request_, GetLoadFlags()).WillRepeatedly(Return(0));
  EXPECT_CALL(*mock_request_, TryResponse(net::HTTP_SERVICE_UNAVAILABLE, ""))
      .WillOnce(Return(RS_HTTP_PROBLEM));
  EXPECT_CALL(*mock_request_, GetRequestManagerUrlType())
      .WillRepeatedly(Return(RMUT_OAUTH2));
  EXPECT_CALL(*mock_request_, GetQueryString()).WillRepeatedly(Return(""));
  EXPECT_CALL(*mock_request_, GetExtraRequestHeaders())
      .WillRepeatedly(Return(std::vector<std::string>{}));

  EXPECT_CALL(
      mock_consumer_,
      OnNetworkRequestFinished(
          static_cast<scoped_refptr<NetworkRequest>>(mock_request_), RS_OK))
      .Times(1);

  fetcher_factory_.SetFakeResponse(kMockOAuth2BaseURL.Resolve(kMockRequestPath),
                                   "", net::HTTP_SERVICE_UNAVAILABLE,
                                   net::URLRequestStatus::SUCCESS);
  request_manager_->StartRequest(mock_request_,
                                 mock_consumer_.weak_ptr_factory_.GetWeakPtr());
  test_task_runner_->FastForwardUntilNoTasksRemain();
  base::RunLoop().RunUntilIdle();

  EXPECT_CALL(*mock_request_, TryResponse(net::HTTP_OK, "mock-response-body"))
      .WillOnce(Return(RS_OK));
  fetcher_factory_.SetFakeResponse(kMockOAuth2BaseURL.Resolve(kMockRequestPath),
                                   "mock-response-body", net::HTTP_OK,
                                   net::URLRequestStatus::SUCCESS);
  request_manager_->StartRequest(mock_request_,
                                 mock_consumer_.weak_ptr_factory_.GetWeakPtr());
  test_task_runner_->FastForwardUntilNoTasksRemain();
  base::RunLoop().RunUntilIdle();
}

TEST_F(NetworkRequestManagerImplTest, SmokeResponseParseProblem) {
  mock_request_ = new NetworkRequestMock;

  EXPECT_CALL(*mock_request_, GetPath())
      .WillRepeatedly(Return(kMockRequestPath));
  EXPECT_CALL(*mock_request_, GetContent())
      .WillRepeatedly(Return("mock=request&content=mock"));
  EXPECT_CALL(*mock_request_, GetHTTPRequestType())
      .WillRepeatedly(Return(net::URLFetcher::POST));
  EXPECT_CALL(*mock_request_, GetRequestContentType())
      .WillRepeatedly(Return("mock-content-type"));
  EXPECT_CALL(*mock_request_, GetLoadFlags()).WillRepeatedly(Return(0));
  EXPECT_CALL(*mock_request_, TryResponse(net::HTTP_OK, "mock-response-body"))
      .WillOnce(Return(RS_PARSE_PROBLEM));
  EXPECT_CALL(*mock_request_, GetRequestManagerUrlType())
      .WillRepeatedly(Return(RMUT_OAUTH2));
  EXPECT_CALL(*mock_request_, GetQueryString()).WillRepeatedly(Return(""));
  EXPECT_CALL(*mock_request_, GetExtraRequestHeaders())
      .WillRepeatedly(Return(std::vector<std::string>{}));

  EXPECT_CALL(
      mock_consumer_,
      OnNetworkRequestFinished(
          static_cast<scoped_refptr<NetworkRequest>>(mock_request_), RS_OK))
      .Times(1);

  fetcher_factory_.SetFakeResponse(kMockOAuth2BaseURL.Resolve(kMockRequestPath),
                                   "mock-response-body", net::HTTP_OK,
                                   net::URLRequestStatus::SUCCESS);
  request_manager_->StartRequest(mock_request_,
                                 mock_consumer_.weak_ptr_factory_.GetWeakPtr());
  test_task_runner_->FastForwardUntilNoTasksRemain();
  base::RunLoop().RunUntilIdle();

  EXPECT_CALL(*mock_request_, TryResponse(net::HTTP_OK, "mock-response-body"))
      .WillOnce(Return(RS_OK));
  fetcher_factory_.SetFakeResponse(kMockOAuth2BaseURL.Resolve(kMockRequestPath),
                                   "mock-response-body", net::HTTP_OK,
                                   net::URLRequestStatus::SUCCESS);
  request_manager_->StartRequest(mock_request_,
                                 mock_consumer_.weak_ptr_factory_.GetWeakPtr());
  test_task_runner_->FastForwardUntilNoTasksRemain();
  base::RunLoop().RunUntilIdle();
}

TEST_F(NetworkRequestManagerImplTest, SmokeServerThrottled) {
  mock_request_ = new NetworkRequestMock;

  EXPECT_CALL(*mock_request_, GetPath())
      .WillRepeatedly(Return(kMockRequestPath));
  EXPECT_CALL(*mock_request_, GetContent())
      .WillRepeatedly(Return("mock=request&content=mock"));
  EXPECT_CALL(*mock_request_, GetHTTPRequestType())
      .WillRepeatedly(Return(net::URLFetcher::POST));
  EXPECT_CALL(*mock_request_, GetRequestContentType())
      .WillRepeatedly(Return("mock-content-type"));
  EXPECT_CALL(*mock_request_, GetLoadFlags()).WillRepeatedly(Return(0));
  EXPECT_CALL(*mock_request_, GetDiagnosticDescription())
      .WillRepeatedly(Return("Request description"));
  EXPECT_CALL(*mock_request_, GetRequestManagerUrlType())
      .WillRepeatedly(Return(RMUT_OAUTH2));
  EXPECT_CALL(*mock_request_, GetQueryString()).WillRepeatedly(Return(""));
  EXPECT_CALL(*mock_request_, GetExtraRequestHeaders())
      .WillRepeatedly(Return(std::vector<std::string>{}));

  EXPECT_CALL(mock_consumer_, GetConsumerName())
      .WillRepeatedly(Return("mock_consumer"));

  const base::TimeDelta retry_after = base::TimeDelta::FromHours(1);
  response_headers_->AddHeader(std::string(kRetryAfter) + ": " +
                               base::IntToString(retry_after.InSeconds()));

  fetcher_factory_.SetFakeResponse(kMockOAuth2BaseURL.Resolve(kMockRequestPath),
                                   kThrottledBody, net::HTTP_TOO_MANY_REQUESTS,
                                   net::URLRequestStatus::SUCCESS);
  request_manager_->StartRequest(mock_request_,
                                 mock_consumer_.weak_ptr_factory_.GetWeakPtr());
  base::Time last_request_time = test_clock_->Now();
  base::Time next_request_time = test_clock_->Now();

  // Fast forward to the posted DoStartRequest, start fetcher.
  ASSERT_TRUE(test_task_runner_->HasPendingTask());
  test_task_runner_->FastForwardBy(test_task_runner_->NextPendingTaskDelay());

  // Run to fetcher complete.
  base::RunLoop().RunUntilIdle();
  base::Time last_response_time = test_clock_->Now();
  next_request_time = next_request_time + retry_after;

  auto actual = request_manager_->GetDiagnosticSnapshot();

  std::unique_ptr<base::DictionaryValue> detail(new base::DictionaryValue);
  std::unique_ptr<base::ListValue> requests(new base::ListValue);
  std::unique_ptr<base::DictionaryValue> request0(new base::DictionaryValue);
  std::unique_ptr<base::DictionaryValue> base_urls(new base::DictionaryValue);
  std::unique_ptr<base::DictionaryValue> response_detail(
      new base::DictionaryValue);

  base_urls->SetString("OAUTH1", kMockOAuth1BaseURL.spec());
  base_urls->SetString("OAUTH2", kMockOAuth2BaseURL.spec());

  response_detail->SetInteger("http_status", 429);
  response_detail->SetString("request_status", "THROTTLED");
  response_detail->SetDouble("response_time", last_response_time.ToJsTime());
  response_detail->SetString("network_error", "net::OK");

  request0->SetString("base_request_url", "OAUTH2");
  request0->SetString("consumer", "mock_consumer");
  request0->SetString("detail", "Request description");
  request0->SetDouble("last_request_time", last_request_time.ToJsTime());
  request0->SetDouble("next_request_time", next_request_time.ToJsTime());
  request0->SetString("path", "/mocked/request/path/");
  request0->SetInteger("request_count", 1);
  request0->SetInteger("type", 1);
  request0->Set("last_response", std::move(response_detail));
  requests->Append(std::move(request0));
  detail->Set("requests", std::move(requests));
  detail->Set("base_request_urls", std::move(base_urls));

  EXPECT_NE(nullptr, actual);
  EXPECT_TRUE(detail->Equals(actual.get())) << "Actual: " << *actual
                                            << "\nExpected: " << *detail;

  // DoStartRequest rescheduled, fetcher started.
  fetcher_factory_.SetFakeResponse(kMockOAuth2BaseURL.Resolve(kMockRequestPath),
                                   "mock-response-body", net::HTTP_OK,
                                   net::URLRequestStatus::SUCCESS);
  ASSERT_TRUE(test_task_runner_->HasPendingTask());
  ASSERT_EQ(retry_after, test_task_runner_->NextPendingTaskDelay());
  test_task_runner_->FastForwardBy(test_task_runner_->NextPendingTaskDelay());

  last_request_time = test_clock_->Now();
  next_request_time = base::Time();
  actual = request_manager_->GetDiagnosticSnapshot();

  detail.reset(new base::DictionaryValue);
  requests.reset(new base::ListValue);
  request0.reset(new base::DictionaryValue);
  base_urls.reset(new base::DictionaryValue);
  response_detail.reset(new base::DictionaryValue);

  base_urls->SetString("OAUTH1", kMockOAuth1BaseURL.spec());
  base_urls->SetString("OAUTH2", kMockOAuth2BaseURL.spec());

  response_detail->SetInteger("http_status", 429);
  response_detail->SetString("request_status", "THROTTLED");
  response_detail->SetDouble("response_time", last_response_time.ToJsTime());
  response_detail->SetString("network_error", "net::OK");

  request0->SetString("base_request_url", "OAUTH2");
  request0->SetString("consumer", "mock_consumer");
  request0->SetString("detail", "Request description");
  request0->SetDouble("last_request_time", last_request_time.ToJsTime());
  request0->SetDouble("next_request_time", next_request_time.ToJsTime());
  request0->SetString("path", "/mocked/request/path/");
  request0->SetInteger("request_count", 2);
  request0->SetInteger("type", 1);
  request0->Set("last_response", std::move(response_detail));
  requests->Append(std::move(request0));
  detail->Set("requests", std::move(requests));
  detail->Set("base_request_urls", std::move(base_urls));

  EXPECT_NE(nullptr, actual);
  EXPECT_TRUE(detail->Equals(actual.get())) << "Actual: " << *actual
                                            << "\nExpected: " << *detail;

  // Run to fetcher complete.
  EXPECT_CALL(*mock_request_, TryResponse(net::HTTP_OK, "mock-response-body"))
      .WillOnce(Return(RS_OK));
  EXPECT_CALL(
      mock_consumer_,
      OnNetworkRequestFinished(
          static_cast<scoped_refptr<NetworkRequest>>(mock_request_), RS_OK))
      .Times(1);
  base::RunLoop().RunUntilIdle();
}

TEST_F(NetworkRequestManagerImplTest,
       SmokeServerNotThrottledNoRetryAfterHeader) {
  mock_request_ = new NetworkRequestMock;

  EXPECT_CALL(*mock_request_, GetPath())
      .WillRepeatedly(Return(kMockRequestPath));
  EXPECT_CALL(*mock_request_, GetContent())
      .WillRepeatedly(Return("mock=request&content=mock"));
  EXPECT_CALL(*mock_request_, GetHTTPRequestType())
      .WillRepeatedly(Return(net::URLFetcher::POST));
  EXPECT_CALL(*mock_request_, GetRequestContentType())
      .WillRepeatedly(Return("mock-content-type"));
  EXPECT_CALL(*mock_request_, GetLoadFlags()).WillRepeatedly(Return(0));
  EXPECT_CALL(*mock_request_, GetDiagnosticDescription())
      .WillRepeatedly(Return("Request description"));
  EXPECT_CALL(*mock_request_, GetRequestManagerUrlType())
      .WillRepeatedly(Return(RMUT_OAUTH2));
  EXPECT_CALL(*mock_request_, GetQueryString()).WillRepeatedly(Return(""));
  EXPECT_CALL(*mock_request_, GetExtraRequestHeaders())
      .WillRepeatedly(Return(std::vector<std::string>{}));

  EXPECT_CALL(mock_consumer_, GetConsumerName())
      .WillRepeatedly(Return("mock_consumer"));

  fetcher_factory_.SetFakeResponse(kMockOAuth2BaseURL.Resolve(kMockRequestPath),
                                   kThrottledBody, net::HTTP_TOO_MANY_REQUESTS,
                                   net::URLRequestStatus::SUCCESS);
  request_manager_->StartRequest(mock_request_,
                                 mock_consumer_.weak_ptr_factory_.GetWeakPtr());
  test_task_runner_->FastForwardUntilNoTasksRemain();
  base::RunLoop().RunUntilIdle();

  auto actual = request_manager_->GetDiagnosticSnapshot();
  const auto initial_backoff = base::TimeDelta::FromSeconds(1);
  auto next_request_time = test_clock_->Now() + initial_backoff;
  auto last_response_time = test_clock_->Now();

  std::unique_ptr<base::DictionaryValue> detail(new base::DictionaryValue);
  std::unique_ptr<base::ListValue> requests(new base::ListValue);
  std::unique_ptr<base::DictionaryValue> request0(new base::DictionaryValue);
  std::unique_ptr<base::DictionaryValue> base_urls(new base::DictionaryValue);
  std::unique_ptr<base::DictionaryValue> response_detail(
      new base::DictionaryValue);

  base_urls->SetString("OAUTH1", kMockOAuth1BaseURL.spec());
  base_urls->SetString("OAUTH2", kMockOAuth2BaseURL.spec());

  response_detail->SetInteger("http_status", 429);
  response_detail->SetString("request_status", "HTTP_PROBLEM");
  response_detail->SetDouble("response_time", last_response_time.ToJsTime());
  response_detail->SetString("network_error", "net::OK");

  request0->SetString("base_request_url", "OAUTH2");
  request0->SetString("consumer", "mock_consumer");
  request0->SetString("detail", "Request description");
  request0->SetDouble("last_request_time", test_clock_->Now().ToJsTime());
  request0->SetDouble("next_request_time", next_request_time.ToJsTime());
  request0->SetString("path", "/mocked/request/path/");
  request0->SetInteger("request_count", 1);
  request0->SetInteger("type", 1);
  request0->Set("last_response", std::move(response_detail));
  requests->Append(std::move(request0));
  detail->Set("requests", std::move(requests));
  detail->Set("base_request_urls", std::move(base_urls));

  EXPECT_NE(nullptr, actual);
  EXPECT_TRUE(detail->Equals(actual.get())) << "Actual: " << *actual
                                            << "\nExpected: " << *detail;

  // Fast forward to the posted retry.
  test_task_runner_->FastForwardUntilNoTasksRemain();

  next_request_time = base::Time();

  detail.reset(new base::DictionaryValue);
  requests.reset(new base::ListValue);
  request0.reset(new base::DictionaryValue);
  base_urls.reset(new base::DictionaryValue);
  response_detail.reset(new base::DictionaryValue);

  base_urls->SetString("OAUTH1", kMockOAuth1BaseURL.spec());
  base_urls->SetString("OAUTH2", kMockOAuth2BaseURL.spec());

  response_detail->SetInteger("http_status", 429);
  response_detail->SetString("request_status", "HTTP_PROBLEM");
  response_detail->SetDouble("response_time", last_response_time.ToJsTime());
  response_detail->SetString("network_error", "net::OK");

  request0->SetString("base_request_url", "OAUTH2");
  request0->SetString("consumer", "mock_consumer");
  request0->SetString("detail", "Request description");
  request0->SetDouble("last_request_time", test_clock_->Now().ToJsTime());
  request0->SetDouble("next_request_time", next_request_time.ToJsTime());
  request0->SetString("path", "/mocked/request/path/");
  request0->SetInteger("request_count", 2);
  request0->SetInteger("type", 1);
  request0->Set("last_response", std::move(response_detail));
  requests->Append(std::move(request0));
  detail->Set("requests", std::move(requests));
  detail->Set("base_request_urls", std::move(base_urls));

  EXPECT_EQ("network_manager", request_manager_->GetDiagnosticName());
  actual = request_manager_->GetDiagnosticSnapshot();
  EXPECT_NE(nullptr, actual);
  EXPECT_TRUE(detail->Equals(actual.get())) << "Actual: " << *actual
                                            << "\nExpected: " << *detail;
}

TEST_F(NetworkRequestManagerImplTest, UsesCorrectUrl) {
  const GURL expected_oauth1_url = kMockOAuth1BaseURL.Resolve(kMockRequestPath);
  const GURL expected_oauth2_url = kMockOAuth2BaseURL.Resolve(kMockRequestPath);

  mock_request_ = new NetworkRequestMock;

  EXPECT_CALL(*mock_request_, GetPath())
      .WillRepeatedly(Return(kMockRequestPath));
  EXPECT_CALL(*mock_request_, GetContent())
      .WillRepeatedly(Return("mock=request&content=mock"));
  EXPECT_CALL(*mock_request_, GetHTTPRequestType())
      .WillRepeatedly(Return(net::URLFetcher::POST));
  EXPECT_CALL(*mock_request_, GetRequestContentType())
      .WillRepeatedly(Return("mock-content-type"));
  EXPECT_CALL(*mock_request_, GetLoadFlags()).WillRepeatedly(Return(0));
  EXPECT_CALL(*mock_request_, TryResponse(net::HTTP_OK, "mock-response-body"))
      .WillRepeatedly(Return(RS_OK));
  EXPECT_CALL(*mock_request_, GetRequestManagerUrlType())
      .WillRepeatedly(Return(RMUT_OAUTH1));
  EXPECT_CALL(*mock_request_, GetQueryString()).WillRepeatedly(Return(""));
  EXPECT_CALL(*mock_request_, GetExtraRequestHeaders())
      .WillRepeatedly(Return(std::vector<std::string>{}));

  EXPECT_CALL(
      mock_consumer_,
      OnNetworkRequestFinished(
          static_cast<scoped_refptr<NetworkRequest>>(mock_request_), RS_OK))
      .Times(1);

  fetcher_factory_.SetFakeResponse(expected_oauth1_url, "mock-response-body",
                                   net::HTTP_OK,
                                   net::URLRequestStatus::SUCCESS);

  request_manager_->StartRequest(mock_request_,
                                 mock_consumer_.weak_ptr_factory_.GetWeakPtr());
  test_task_runner_->FastForwardUntilNoTasksRemain();
  EXPECT_EQ(expected_oauth1_url, fetcher_->GetURL());
  base::RunLoop().RunUntilIdle();

  mock_request_ = new NetworkRequestMock;

  EXPECT_CALL(*mock_request_, GetPath())
      .WillRepeatedly(Return(kMockRequestPath));
  EXPECT_CALL(*mock_request_, GetContent())
      .WillRepeatedly(Return("mock=request&content=mock"));
  EXPECT_CALL(*mock_request_, GetHTTPRequestType())
      .WillRepeatedly(Return(net::URLFetcher::POST));
  EXPECT_CALL(*mock_request_, GetRequestContentType())
      .WillRepeatedly(Return("mock-content-type"));
  EXPECT_CALL(*mock_request_, GetLoadFlags()).WillRepeatedly(Return(0));
  EXPECT_CALL(*mock_request_, TryResponse(net::HTTP_OK, "mock-response-body"))
      .WillRepeatedly(Return(RS_OK));
  EXPECT_CALL(*mock_request_, GetRequestManagerUrlType())
      .WillRepeatedly(Return(RMUT_OAUTH2));
  EXPECT_CALL(*mock_request_, GetQueryString()).WillRepeatedly(Return(""));
  EXPECT_CALL(*mock_request_, GetExtraRequestHeaders())
      .WillRepeatedly(Return(std::vector<std::string>{}));

  EXPECT_CALL(
      mock_consumer_,
      OnNetworkRequestFinished(
          static_cast<scoped_refptr<NetworkRequest>>(mock_request_), RS_OK))
      .Times(1);

  fetcher_factory_.SetFakeResponse(expected_oauth2_url, "mock-response-body",
                                   net::HTTP_OK,
                                   net::URLRequestStatus::SUCCESS);

  request_manager_->StartRequest(mock_request_,
                                 mock_consumer_.weak_ptr_factory_.GetWeakPtr());
  test_task_runner_->FastForwardUntilNoTasksRemain();
  EXPECT_EQ(expected_oauth2_url, fetcher_->GetURL());
  base::RunLoop().RunUntilIdle();
}

TEST_F(NetworkRequestManagerImplTest, BlockInsecureConnection) {
  NetworkRequestManagerImpl::RequestUrlsConfig url_config;
  url_config[RMUT_OAUTH1] = std::make_pair(kMockOAuth1BaseURLInsecure, false);
  url_config[RMUT_OAUTH2] = std::make_pair(kMockOAuth2BaseURL, false);
  CreateRequestManager(url_config);

  mock_request_ = new NetworkRequestMock;

  EXPECT_CALL(*mock_request_, GetPath())
      .WillRepeatedly(Return(kMockRequestPath));
  EXPECT_CALL(*mock_request_, GetContent())
      .WillRepeatedly(Return("mock=request&content=mock"));
  EXPECT_CALL(*mock_request_, GetHTTPRequestType())
      .WillRepeatedly(Return(net::URLFetcher::POST));
  EXPECT_CALL(*mock_request_, GetRequestContentType())
      .WillRepeatedly(Return("mock-content-type"));
  EXPECT_CALL(*mock_request_, GetLoadFlags()).WillRepeatedly(Return(0));
  EXPECT_CALL(*mock_request_, TryResponse(net::HTTP_OK, "mock-response-body"))
      .WillRepeatedly(Return(RS_OK));
  EXPECT_CALL(*mock_request_, GetRequestManagerUrlType())
      .WillRepeatedly(Return(RMUT_OAUTH1));
  EXPECT_CALL(*mock_request_, GetQueryString()).WillRepeatedly(Return(""));
  EXPECT_CALL(*mock_request_, GetExtraRequestHeaders())
      .WillRepeatedly(Return(std::vector<std::string>{
          "header_name:header_value", "other_header:other_value"}));

  EXPECT_CALL(mock_consumer_,
              OnNetworkRequestFinished(
                  static_cast<scoped_refptr<NetworkRequest>>(mock_request_),
                  RS_INSECURE_CONNECTION_FORBIDDEN))
      .Times(1);

  fetcher_factory_.SetFakeResponse(
      kMockOAuth1BaseURLInsecure.Resolve(kMockRequestPath),
      "mock-response-body", net::HTTP_OK, net::URLRequestStatus::SUCCESS);

  request_manager_->StartRequest(mock_request_,
                                 mock_consumer_.weak_ptr_factory_.GetWeakPtr());
  test_task_runner_->FastForwardUntilNoTasksRemain();

  base::RunLoop().RunUntilIdle();
}

TEST_F(NetworkRequestManagerImplTest,
       RedirectNoLocationFedDirectlyToTryResponse) {
  mock_request_ = new NetworkRequestMock;

  EXPECT_CALL(*mock_request_, GetPath())
      .WillRepeatedly(Return(kMockRequestPath));
  EXPECT_CALL(*mock_request_, GetContent())
      .WillRepeatedly(Return("mock=request&content=mock"));
  EXPECT_CALL(*mock_request_, GetHTTPRequestType())
      .WillRepeatedly(Return(net::URLFetcher::POST));
  EXPECT_CALL(*mock_request_, GetRequestContentType())
      .WillRepeatedly(Return("mock-content-type"));
  EXPECT_CALL(*mock_request_, GetLoadFlags()).WillRepeatedly(Return(0));
  EXPECT_CALL(*mock_request_,
              TryResponse(net::HTTP_SEE_OTHER, "mock-response-body"))
      .WillRepeatedly(Return(RS_OK));
  EXPECT_CALL(*mock_request_, GetRequestManagerUrlType())
      .WillRepeatedly(Return(RMUT_OAUTH1));
  EXPECT_CALL(*mock_request_, GetQueryString()).WillRepeatedly(Return(""));
  EXPECT_CALL(*mock_request_, GetExtraRequestHeaders())
      .WillRepeatedly(Return(std::vector<std::string>{}));

  EXPECT_CALL(
      mock_consumer_,
      OnNetworkRequestFinished(
          static_cast<scoped_refptr<NetworkRequest>>(mock_request_), RS_OK))
      .Times(1);

  // The original HTTP status (303 See Other) with CANCELLED result, this is how
  // the URLFetcher is supposed to behave when it sees a redirect and
  // SetStopOnRedirect(true) was called.
  fetcher_factory_.SetFakeResponse(kMockOAuth1BaseURL.Resolve(kMockRequestPath),
                                   "mock-response-body", net::HTTP_SEE_OTHER,
                                   net::URLRequestStatus::CANCELED);

  request_manager_->StartRequest(mock_request_,
                                 mock_consumer_.weak_ptr_factory_.GetWeakPtr());
  test_task_runner_->FastForwardUntilNoTasksRemain();

  base::RunLoop().RunUntilIdle();
}

TEST_F(NetworkRequestManagerImplTest,
       RedirectWithLocationFedDirectlyToTryResponse) {
  mock_request_ = new NetworkRequestMock;

  EXPECT_CALL(*mock_request_, GetPath())
      .WillRepeatedly(Return(kMockRequestPath));
  EXPECT_CALL(*mock_request_, GetContent())
      .WillRepeatedly(Return("mock=request&content=mock"));
  EXPECT_CALL(*mock_request_, GetHTTPRequestType())
      .WillRepeatedly(Return(net::URLFetcher::POST));
  EXPECT_CALL(*mock_request_, GetRequestContentType())
      .WillRepeatedly(Return("mock-content-type"));
  EXPECT_CALL(*mock_request_, GetLoadFlags()).WillRepeatedly(Return(0));
  EXPECT_CALL(*mock_request_,
              TryResponse(net::HTTP_SEE_OTHER, "mock-response-body"))
      .WillRepeatedly(Return(RS_OK));
  EXPECT_CALL(*mock_request_, GetRequestManagerUrlType())
      .WillRepeatedly(Return(RMUT_OAUTH1));
  EXPECT_CALL(*mock_request_, GetQueryString()).WillRepeatedly(Return(""));
  EXPECT_CALL(*mock_request_, GetExtraRequestHeaders())
      .WillRepeatedly(Return(std::vector<std::string>{}));

  EXPECT_CALL(
      mock_consumer_,
      OnNetworkRequestFinished(
          static_cast<scoped_refptr<NetworkRequest>>(mock_request_), RS_OK))
      .Times(1);

  response_headers_->AddHeader("Location: http://opera.com/");

  // The original HTTP status (303 See Other) with CANCELLED result, this is how
  // the URLFetcher is supposed to behave when it sees a redirect and
  // SetStopOnRedirect(true) was called.
  fetcher_factory_.SetFakeResponse(kMockOAuth1BaseURL.Resolve(kMockRequestPath),
                                   "mock-response-body", net::HTTP_SEE_OTHER,
                                   net::URLRequestStatus::CANCELED);

  request_manager_->StartRequest(mock_request_,
                                 mock_consumer_.weak_ptr_factory_.GetWeakPtr());
  test_task_runner_->FastForwardUntilNoTasksRemain();

  base::RunLoop().RunUntilIdle();
}

}  // namespace oauth2
}  // namespace opera
