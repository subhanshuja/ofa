// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sync_auth_data_updater.h"

#include <string>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/sha1.h"
#include "base/strings/stringprintf.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_status.h"

#include "common/net/sensitive_url_request_user_data.h"
#include "common/sync/sync_config.h"
#include "common/sync/sync_login_data.h"
#include "common/sync/sync_login_error_data.h"

namespace opera {

namespace {
const int kAuthNonExpiredTokenRenewedErrCode = 428;
const char kSignatureBaseStringFormat[] =
    "consumer_key=%s&old_token=%s&service=%sX%s";
const char kRenewalURLFormat[] =
    "%saccount/access-token/renewal"
    "?consumer_key=%s&old_token=%s&service=%s&signature=%s";

GURL BuildRenewalURL(const std::string& host_name,
                     const std::string& client_key,
                     const std::string& auth_service,
                     const std::string& client_secret,
                     const std::string& old_token) {
  const std::string signature_base_string = base::StringPrintf(
      kSignatureBaseStringFormat, client_key.c_str(), old_token.c_str(),
      auth_service.c_str(), client_secret.c_str());
  const std::string raw_signature = base::SHA1HashString(signature_base_string);
  const std::string signature = base::ToLowerASCII(
      base::HexEncode(raw_signature.data(), raw_signature.size()));
  const std::string url = base::StringPrintf(
      kRenewalURLFormat, host_name.c_str(), client_key.c_str(),
      old_token.c_str(), auth_service.c_str(), signature.c_str());
  return GURL(url);
}

}  // namespace

SyncAuthDataUpdater::SyncAuthDataUpdater(
    net::URLRequestContextGetter* request_context_getter,
    const AccountAuthData& old_auth_data)
    : renewal_url_(BuildRenewalURL(opera::SyncConfig::AuthServerURL().spec(),
                                   opera::SyncConfig::ClientKey(),
                                   opera::SyncConfig::SyncAuthService(),
                                   opera::SyncConfig::ClientSecret(),
                                   old_auth_data.token)),
      request_context_getter_(request_context_getter),
      old_auth_data_(old_auth_data) {}

SyncAuthDataUpdater::~SyncAuthDataUpdater() {}

void SyncAuthDataUpdater::RequestAuthData(
    opera_sync::OperaAuthProblem problem,
    const RequestAuthDataSuccessCallback& success_callback,
    const RequestAuthDataFailureCallback& failure_callback) {
  DCHECK(!fetcher_);

  auth_problem_ = problem;

  success_callback_ = success_callback;
  failure_callback_ = failure_callback;

  fetcher_ = net::URLFetcher::Create(
      // Pass a dummy ID (15).  This causes an URLFetcherFactory to be called,
      // if one is registered.  We register a test URLFetcherFactory in tests.
      // The ID is ignored on Mobile Mini platforms that have their own
      // implementation of net
      15, renewal_url_, net::URLFetcher::GET, this);
  fetcher_->SetRequestContext(request_context_getter_);
  fetcher_->SetLoadFlags(net::LOAD_DISABLE_CACHE |
                         net::LOAD_DO_NOT_SAVE_COOKIES |
                         net::LOAD_DO_NOT_SEND_COOKIES);
  fetcher_->SetURLRequestUserData(
      SensitiveURLRequestUserData::kUserDataKey,
      base::Bind(&SensitiveURLRequestUserData::Create));

  VLOG(4) << "Sending token renewal request: " << fetcher_->GetOriginalURL();

  fetcher_->Start();
}

void SyncAuthDataUpdater::OnURLFetchComplete(const net::URLFetcher* source) {
  // We're sending the token renew request here. A proper answer from the
  // auth.opera.com service is a HTTP 200 with a JSON string describing the
  // actual result that will be successfully parsed either by
  // JSONToSyncLoginData() or JSONToSyncLoginErrorData().
  // In particular, HTTP responses other than 200 do not mean an auth failure
  // but rather an auth infrastructure failure.

  DCHECK_EQ(source, fetcher_.get());

  std::string response;
  if (!fetcher_->GetResponseAsString(&response)) {
    NOTREACHED() << "Misconfigured URLFetcher";
  }
  const net::URLRequestStatus status = fetcher_->GetStatus();
  const int response_code = fetcher_->GetResponseCode();
  fetcher_.reset();

  VLOG_IF(4, !response.empty()) << "Response:\n" << response;

  if (status.status() != net::URLRequestStatus::SUCCESS) {
    std::string error_string = net::ErrorToString(status.error());
    const std::string message =
        base::StringPrintf("%d (%s)", status.error(), error_string.c_str());
    VLOG(1) << "Request status: " << status.status() << ", error: " << message;

    failure_callback_.Run(account_util::ADUE_NETWORK_ERROR,
                          account_util::AOCE_NO_ERROR, message, auth_problem_);
    return;
  }

  if (response_code != net::HTTP_OK) {
    const std::string message = base::StringPrintf(
        "%d (%s)", response_code,
        net::GetHttpReasonPhrase(net::HttpStatusCode(response_code)));
    VLOG(1) << "Response code: " << message;

    failure_callback_.Run(account_util::ADUE_HTTP_ERROR,
                          account_util::AOCE_NO_ERROR, message, auth_problem_);
    return;
  }

  std::string parse_error;

  SyncLoginData new_login_data;
  if (!JSONToSyncLoginData(response, &new_login_data, &parse_error)) {
    SyncLoginErrorData error_data;
    if (!JSONToSyncLoginErrorData(response, &error_data, &parse_error)) {
      VLOG(1) << "Failed to parse the response: " << parse_error;

      failure_callback_.Run(account_util::ADUE_PARSE_ERROR,
                            account_util::AOCE_NO_ERROR, parse_error,
                            auth_problem_);
    } else {
      VLOG(1) << "Auth error: " << error_data.code << " (" << error_data.message
              << ')';

      if (!opera::SyncConfig::ShouldUseSmartTokenHandling() &&
          error_data.code == kAuthNonExpiredTokenRenewedErrCode) {
        // Due to interaction between the TICL service and the sync core it is
        // possible that we will be attempting to refresh a non-expired token.
        // We consider such a case a valid scenario that is in fact ignored and
        // the same token is resued in the client code.
        VLOG(1) << "Renewing a nonexpired token is not an error, reusing old "
                   "auth data.";
        success_callback_.Run(old_auth_data_, auth_problem_);
      } else {
        account_util::AuthOperaComError auth_code =
            account_util::AuthOperaComErrorFromInt(error_data.code);
        failure_callback_.Run(account_util::ADUE_AUTH_ERROR, auth_code,
                              error_data.message, auth_problem_);
      }
    }
    return;
  }

  success_callback_.Run(new_login_data.auth_data, auth_problem_);
}

AccountAuthDataFetcher* CreateAuthDataUpdater(
    net::URLRequestContextGetter* request_context_getter,
    const AccountAuthData& old_auth_data) {
  return new SyncAuthDataUpdater(request_context_getter, old_auth_data);
}

}  // namespace opera
