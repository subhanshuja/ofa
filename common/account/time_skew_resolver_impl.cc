// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/account/time_skew_resolver_impl.h"

#include <string>

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "base/time/time.h"
#include "net/url_request/url_fetcher.h"
#include "url/gurl.h"

#include "common/sync/sync_config.h"

namespace opera {

namespace {

const char kRequestMimeType[] = "application/x-www-form-urlencoded";
const char kRequestTemplate[] = "localtime=";

const char kJSONResponseStatus[] = "status";
const char kJSONResponseCode[] = "code";
const char kJSONResponseMessage[] = "message";
const char kJSONResponseDifference[] = "diff";

const int64_t kDiffNotUsed = 0;

GURL GetTimeCheckURL(const GURL& auth_server_url) {
  return auth_server_url.Resolve("/service/validation/clock");
}

bool GetResultJSON(const net::URLFetcher& request, std::string* result) {
  return request.GetResponseAsString(result);
}

TimeSkewResolver::ResultValue ParseResultJSON(
    const std::string& json_response) {
  VLOG(1) << "Parsing time skew response form server: " << json_response;

  std::unique_ptr<base::Value> parsed_response;
  {
    int dummy_json_error_code;
    std::string json_error_message;

    parsed_response =
        base::JSONReader::ReadAndReturnError(json_response,
                                             base::JSON_ALLOW_TRAILING_COMMAS,
                                             &dummy_json_error_code,
                                             &json_error_message);

    if (!parsed_response ||
        parsed_response->GetType() != base::Value::TYPE_DICTIONARY) {
      return TimeSkewResolver::ResultValue(
          TimeSkewResolver::QueryResult::INVALID_RESPONSE_ERROR,
          kDiffNotUsed,
          json_error_message);
    }
  }

  base::DictionaryValue* dictionary;
  parsed_response->GetAsDictionary(&dictionary);

  if (dictionary->HasKey(kJSONResponseStatus) &&
      dictionary->HasKey(kJSONResponseCode) &&
      dictionary->HasKey(kJSONResponseMessage)) {
    std::string status;
    dictionary->GetString(kJSONResponseStatus, &status);
    int code;
    dictionary->GetInteger(kJSONResponseCode, &code);
    std::string message;
    dictionary->GetString(kJSONResponseMessage, &message);

    if (status == "success" &&
        code == static_cast<int>(TimeSkewResolver::QueryResult::TIME_OK)) {
      return TimeSkewResolver::ResultValue(
          TimeSkewResolver::QueryResult::TIME_OK, kDiffNotUsed, message);
    } else if (status == "error") {
      if (code == static_cast<int>(TimeSkewResolver::QueryResult::TIME_SKEW)) {
        if (dictionary->HasKey(kJSONResponseDifference)) {
          double difference;
          if (dictionary->GetDouble(kJSONResponseDifference, &difference)) {
            return TimeSkewResolver::ResultValue(
                TimeSkewResolver::QueryResult::TIME_SKEW,
                static_cast<int64_t>(difference), message);
          } else {
            return TimeSkewResolver::ResultValue(
                TimeSkewResolver::QueryResult::INVALID_RESPONSE_ERROR, 0,
                "Could not parse time skew value as a number");
          }
        }
      } else {
        return TimeSkewResolver::ResultValue(
            TimeSkewResolver::QueryResult::BAD_REQUEST_ERROR,
            kDiffNotUsed,
            message);
      }
    }
  }

  return TimeSkewResolver::ResultValue(
      TimeSkewResolver::QueryResult::INVALID_RESPONSE_ERROR,
      kDiffNotUsed,
      "Unexpected format of obtained message");
}

bool IsRequestInvalid(const net::URLFetcher* request) {
  return !request ||
         request->GetResponseCode() == net::URLFetcher::RESPONSE_CODE_INVALID;
}

TimeSkewResolver::ResultValue CalculateResult(const net::URLFetcher* request) {
  if (IsRequestInvalid(request)) {
    return TimeSkewResolver::ResultValue(
        TimeSkewResolver::QueryResult::NO_RESPONSE_ERROR,
        kDiffNotUsed,
        "No response obtained from server");
  }

  std::string json;
  if (GetResultJSON(*request, &json)) {
    VLOG(1) << "Time skew result fetched: " << json;
    return ParseResultJSON(json);
  }

  return TimeSkewResolver::ResultValue(
      TimeSkewResolver::QueryResult::INVALID_RESPONSE_ERROR,
      kDiffNotUsed,
      "Illformed response obtained from server (not valid JSON)");
}

}  // anonymous namespace

TimeSkewResolverImpl::TimeSkewResolverImpl(
    URLFetcherFactory fetcher_factory,
    net::URLRequestContextGetter* request_context,
    const GURL& auth_server_url)
    : fetcher_factory_(fetcher_factory),
      request_context_(request_context),
      time_check_url_(GetTimeCheckURL(auth_server_url)) {
  DCHECK(!fetcher_factory_.is_null());
  DCHECK(request_context_);
  DCHECK(time_check_url_.is_valid());
}

TimeSkewResolverImpl::~TimeSkewResolverImpl() {
  DCHECK(CalledOnValidThread());
}

void TimeSkewResolverImpl::RequestTimeSkew(ResultCallback callback) {
  DCHECK(CalledOnValidThread());

  if (ongoing_request_) {
    VLOG(1) << "New time skew request while old one hasn't finished"
            << " - previous request will be terminated";
  }

  const auto current_timestamp =
      (base::Time::Now() - base::Time::UnixEpoch()).InSeconds();
  const auto request_data =
      std::string(kRequestTemplate) + base::Int64ToString(current_timestamp);

  VLOG(1) << "Starting time skew request: " << request_data;

  callback_ = callback;
  ongoing_request_ =
      fetcher_factory_.Run(0, time_check_url_, net::URLFetcher::POST, this);
  ongoing_request_->SetRequestContext(request_context_);
  ongoing_request_->SetUploadData(kRequestMimeType, request_data);
  ongoing_request_->SetStopOnRedirect(true);
  ongoing_request_->Start();
}

void TimeSkewResolverImpl::OnURLFetchComplete(const net::URLFetcher* request) {
  DCHECK(CalledOnValidThread());

  TimeSkewResolver::ResultValue result = CalculateResult(request);
  callback_.Run(result);

  ongoing_request_.reset();
}

// static
std::unique_ptr<TimeSkewResolver> TimeSkewResolverImpl::Create(
    net::URLRequestContextGetter* time_skew_request_context,
    const GURL& auth_server_url) {
  typedef TimeSkewResolverImpl::URLFetcherFactory::RunType* RequestFactoryType;

  std::unique_ptr<TimeSkewResolver> resolver(new TimeSkewResolverImpl(
      base::Bind(static_cast<RequestFactoryType>(&net::URLFetcher::Create)),
      time_skew_request_context, auth_server_url));

  return resolver;
}

}  // namespace opera
