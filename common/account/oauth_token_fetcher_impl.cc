// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/account/oauth_token_fetcher_impl.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_status.h"
#include "url/gurl.h"

#include "common/account/account_auth_data.h"
#include "common/net/sensitive_url_request_user_data.h"
#include "common/sync/sync_config.h"

namespace opera {

namespace {

std::string Escape(const std::string& input) {
  return net::EscapeUrlEncodedData(input, false);
}

std::string Unescape(const std::string& input) {
  const unsigned rules =
    net::UnescapeRule::SPACES |
    net::UnescapeRule::REPLACE_PLUS_WITH_SPACE |
    net::UnescapeRule::PATH_SEPARATORS |
    net::UnescapeRule::URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS;
  return net::UnescapeURLComponent(input, rules);
}

}  // namespace

OAuthTokenFetcherImpl::OAuthTokenFetcherImpl(
    const std::string& auth_server_url,
    const std::string& client_key,
    net::URLRequestContextGetter* request_context_getter)
    : auth_server_url_(auth_server_url),
      client_key_(client_key),
      request_context_getter_(request_context_getter) {
  DCHECK(request_context_getter_ != NULL);
}

OAuthTokenFetcherImpl::~OAuthTokenFetcherImpl() {}

void OAuthTokenFetcherImpl::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK_EQ(source, fetcher_.get());

  const net::URLRequestStatus status = fetcher_->GetStatus();
  const bool success = status.is_success();
  const int response_code = fetcher_->GetResponseCode();

  std::string response;
  fetcher_->GetResponseAsString(&response);
  fetcher_.reset();

  VLOG_IF(3, response_code != net::HTTP_OK) << "Response code: "
                                            << response_code;
  VLOG_IF(3, !success) << "Request status: " << status.status()
                       << ", error code: " << status.error();
  DVLOG_IF(4, !response.empty()) << "Response:\n" << response;

  if (!success || response_code != net::HTTP_OK) {
    account_util::AuthDataUpdaterError error_code = account_util::ADUE_NO_ERROR;
    std::string message;
    if (!success) {
      DCHECK_NE(status.error(), 0);
      std::string error_string = net::ErrorToString(status.error());
      message =
          base::StringPrintf("%d (%s)", status.error(), error_string.c_str());
      error_code = account_util::ADUE_NETWORK_ERROR;
    } else {
      DCHECK_NE(response_code, net::HTTP_OK);
      message = base::StringPrintf(
          "%d (%s)", response_code,
          net::GetHttpReasonPhrase(net::HttpStatusCode(response_code)));
      error_code = account_util::ADUE_HTTP_ERROR;
    }
    failure_callback_.Run(error_code, account_util::AOCE_NO_ERROR, message,
                          auth_problem_);
  } else {
    ParseServerResponse(response);
  }
}

void OAuthTokenFetcherImpl::Start() {
  DCHECK(fetcher_.get() == NULL);

  fetcher_ = net::URLFetcher::Create(GURL(auth_server_url_),
                                     net::URLFetcher::POST, this);
  fetcher_->SetRequestContext(request_context_getter_);
  fetcher_->SetLoadFlags(net::LOAD_DO_NOT_SAVE_COOKIES |
                         net::LOAD_DO_NOT_SEND_COOKIES |
                         net::LOAD_DISABLE_CACHE);

  const std::string data =
      base::StringPrintf("x_username=%s&x_password=%s&x_consumer_key=%s",
                         Escape(user_name_).c_str(), Escape(password_).c_str(),
                         Escape(client_key_).c_str());
  fetcher_->SetUploadData("application/x-www-form-urlencoded", data);
  fetcher_->SetURLRequestUserData(
      SensitiveURLRequestUserData::kUserDataKey,
      base::Bind(&SensitiveURLRequestUserData::Create));

  fetcher_->Start();
}

void OAuthTokenFetcherImpl::ParseServerResponse(const std::string& response) {
  // Response parameters will be parsed into {key, value} pairs.
  // The response is considered "successful" if:
  //   1. It doesn't contain an "error" parameter.
  //   2. It contains both the "oauth_token" and "oauth_token_secret"
  //      parameters, in any order.
  //
  // Sample error response:
  //   error=User%20authentication%20failed&success=0
  // Sample success response:
  //   oauth_token=ATdC5vEqt3XDFVMgbNg4naTJuLN5C0Io&oauth_token_secret=7YMg9AHqx7A7SlmxmsD6te5ZjZaXqnaY

  std::vector<std::pair<std::string, std::string>> parameters;
  base::SplitStringIntoKeyValuePairs(response, '=', '&', &parameters);

  int error_index = -1;
  int auth_token_index = -1;
  int auth_secret_index = -1;

  for (unsigned i = 0; i < parameters.size(); ++i) {
    const std::string& key = parameters[i].first;
    if (key == "error") {
      error_index = i;
      break;
    } else if (key == "oauth_token") {
      auth_token_index = i;
    } else if (key == "oauth_token_secret") {
      auth_secret_index = i;
    }
  }

  if (error_index == -1 && auth_token_index != -1 && auth_secret_index != -1) {
    AccountAuthData auth_data;
    auth_data.token = Unescape(parameters[auth_token_index].second);
    auth_data.token_secret = Unescape(parameters[auth_secret_index].second);
    if (SyncConfig::ShouldUseAuthTokenRecovery() &&
        (auth_data.token.empty() || auth_data.token_secret.empty())) {
      failure_callback_.Run(account_util::ADUE_PARSE_ERROR,
                            account_util::AOCE_NO_ERROR,
                            "Incomplete auth data obtained.", auth_problem_);
    } else {
      success_callback_.Run(auth_data, auth_problem_);
    }
  } else if (error_index != -1 && auth_token_index == -1 &&
             auth_secret_index == -1) {
    // The infrastructure could probably return more specific auth errors
    // here, but it doesn't.
    failure_callback_.Run(
        account_util::ADUE_AUTH_ERROR,
        account_util::AOCE_AUTH_ERROR_SIMPLE_REQUEST,
        error_index != -1 ? Unescape(parameters[error_index].second) : "",
        auth_problem_);
  } else {
    failure_callback_.Run(account_util::ADUE_PARSE_ERROR,
                          account_util::AOCE_NO_ERROR,
                          "Cannot parse server response.", auth_problem_);
  }
}

}  // namespace opera
