// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2017 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#include "common/auth/auth_token_fetcher.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "net/base/escape.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_status.h"
#include "url/gurl.h"

#include "common/auth/auth_service_constants.h"
#include "common/auth/auth_token_consumer.h"

namespace {

std::string BuildHeader(const std::string& name, const std::string& value) {
  return name + ": " + value;
}

// Error codes as returned in the X-Opera-Auth-ErrCode header. See
// https://wiki.opera.com/display/AUTH/AuthToken
enum AuthErrorCode {
  NOT_AUTHORIZED_REQUEST = 420,
  COULD_NOT_GENERATE_AUTH_TOKEN = 429,
  REQUEST_RATE_LIMITED = 430,
};

}  // namespace

namespace opera {

const char kAuth[] = "https://auth.opera.com";
const char kLoginURL[] = "/account/login?get_opera_access_token=1";

AuthTokenFetcher::AuthTokenFetcher(
    net::URLRequestContextGetter* request_context_getter)
    : state_(STATE_IDLE),
      request_context_getter_(request_context_getter),
      url_fetcher_id_(0) {}

AuthTokenFetcher::~AuthTokenFetcher() {}

void AuthTokenFetcher::RequestAuthTokenForOAuth2(
    const std::string& username,
    const std::string& password,
    const base::WeakPtr<AuthTokenConsumer>& consumer) {
  DCHECK_EQ(state_, STATE_IDLE);
  DCHECK(!fetcher_);
  state_ = STATE_FETCH_CSRF_TOKEN;

  username_ = username;
  password_ = password;
  consumer_ = consumer;

  GURL login(kAuth + std::string(kLoginURL));
  fetcher_ = net::URLFetcher::Create(url_fetcher_id_++, login,
                                     net::URLFetcher::GET, this);
  fetcher_->SetRequestContext(request_context_getter_);
  fetcher_->AddExtraRequestHeader(BuildHeader(kMobileClientHeader, "ofa"));
  fetcher_->Start();
}

void AuthTokenFetcher::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK_EQ(source, fetcher_.get());

  if (!consumer_) {
    // The consumer dropped out and we can safely ignore the response
    fetcher_.reset();
    state_ = STATE_IDLE;
    return;
  }

  const net::URLRequestStatus::Status status = source->GetStatus().status();
  if (status != net::URLRequestStatus::SUCCESS) {
    DCHECK_NE(status, net::URLRequestStatus::IO_PENDING);
    if (status == net::URLRequestStatus::CANCELED)
      return;

    if (status == net::URLRequestStatus::FAILED) {
      int error = source->GetStatus().error();
      consumer_->OnAuthError(CONNECTION_FAILED, net::ErrorToString(error));

      fetcher_.reset();
      state_ = STATE_IDLE;
      return;
    }
  }

  if (source->GetResponseCode() != net::HTTP_OK) {
    net::HttpStatusCode status =
        static_cast<net::HttpStatusCode>(source->GetResponseCode());
    LOG(ERROR) << "Bad Http response: " << status;
    consumer_->OnAuthError(
        UNEXPECTED_RESPONSE,
        base::IntToString(status) + ": " + net::GetHttpReasonPhrase(status));
    fetcher_.reset();
    state_ = STATE_IDLE;
    return;
  }

  switch (state_) {
    case STATE_FETCH_CSRF_TOKEN:
      HandleInitialGET(source);
      break;
    case STATE_FETCH_AUTH_TOKEN:
      HandleAuthenticationResponse(source);
      break;
    case STATE_IDLE:
    default:
      NOTREACHED();
      break;
  }
}

void AuthTokenFetcher::HandleInitialGET(const net::URLFetcher* source) {
  DCHECK_EQ(state_, STATE_FETCH_CSRF_TOKEN);

  // Move fetcher to this scope to ensure it is removed
  std::unique_ptr<net::URLFetcher> fetcher = std::move(fetcher_);

  net::HttpResponseHeaders* headers = source->GetResponseHeaders();
  DCHECK(headers);

  std::string csrf_token;
  if (!headers->GetNormalizedHeader(kCSRFTokenHeader, &csrf_token)) {
    consumer_->OnAuthError(UNEXPECTED_RESPONSE, "Missing csrf token header");
    state_ = STATE_IDLE;
    return;
  }

  RequestAuthTokenWithCSRFToken(csrf_token);
}

void AuthTokenFetcher::HandleAuthenticationResponse(
    const net::URLFetcher* source) {
  DCHECK_EQ(state_, STATE_FETCH_AUTH_TOKEN);

  // Move fetcher to this scope to ensure it is removed
  std::unique_ptr<net::URLFetcher> fetcher = std::move(fetcher_);

  net::HttpResponseHeaders* headers = source->GetResponseHeaders();
  DCHECK(headers);

  std::string error_code_str;
  bool has_error =
      headers->GetNormalizedHeader(kErrorCodeHeader, &error_code_str);
  if (has_error) {
    int error_code;
    base::StringToInt(error_code_str, &error_code);
    if (error_code == REQUEST_RATE_LIMITED) {
      VLOG(2) << "Got 430: schedule retry";
      // TODO(ingemara):
      NOTIMPLEMENTED();
    }

    // TODO(ingemara): AUTH-970: When server people are ready, look for header
    // indicating credential problems rather than making assumptions.
    std::string error_message;
    headers->GetNormalizedHeader(kErrorMessageHeader, &error_message);
    LOG(ERROR) << "Bad response from auth service: " << error_code
               << (error_message.empty() ? "" : "(" + error_message + ")");

    consumer_->OnAuthError(INTERNAL_ERROR, error_message);
    state_ = STATE_IDLE;
    return;
  }

  int user_id;
  std::string user_id_str;
  std::string user_email;
  std::string username;
  std::string auth_token;

  bool found = headers->GetNormalizedHeader(kAuthTokenHeader, &auth_token);
  found &= headers->GetNormalizedHeader(kUserIDHeader, &user_id_str);
  found &= base::StringToInt(user_id_str, &user_id);
  found &= headers->GetNormalizedHeader(kUserEmailHeader, &user_email);
  found &= headers->GetNormalizedHeader(kUserNameHeader, &username);

  if (!found) {
    // TODO(ingemara): INVALID_CREDENTIALS here look odd, but currently the only
    // way to be sure that invalid credentials were supplied is by parsing and
    // interpreting the response body. See TODO above.
    consumer_->OnAuthError(INVALID_CREDENTIALS, "Missing/invalid header(s)");
    state_ = STATE_IDLE;
    return;
  }

  VLOG(2) << "Received auth token: " << auth_token << " for " << username;
  auto user_data = base::MakeUnique<AuthTokenConsumer::UserData>(
      username, user_email, user_id);
  consumer_->OnAuthSuccess(auth_token, std::move(user_data));

  state_ = STATE_IDLE;
}

void AuthTokenFetcher::RequestAuthTokenWithCSRFToken(
    const std::string& csrf_token) {
  state_ = STATE_FETCH_AUTH_TOKEN;
  GURL login(kAuth + std::string(kLoginURL));
  fetcher_ = net::URLFetcher::Create(url_fetcher_id_++, login,
                                     net::URLFetcher::POST, this);
  fetcher_->SetRequestContext(request_context_getter_);
  fetcher_->AddExtraRequestHeader(BuildHeader(kMobileClientHeader, "ofa"));

  std::string upload_data = base::StringPrintf(
      "email=%s"
      "&password=%s"
      "&csrf_token=%s",
      net::EscapeUrlEncodedData(username_, true).c_str(),
      net::EscapeUrlEncodedData(password_, true).c_str(),
      net::EscapeUrlEncodedData(csrf_token, true).c_str());

  username_.clear();
  password_.clear();

  fetcher_->SetUploadData("application/x-www-form-urlencoded", upload_data);
  fetcher_->Start();
}

}  // namespace opera
