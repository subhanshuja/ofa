// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/network/migration_token_request.h"

#include <memory>
#include <string>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "google_apis/gaia/oauth_request_signer.h"
#include "net/base/load_flags.h"

#include "common/oauth2/network/migration_token_response.h"
#include "common/oauth2/network/request_vars_encoder_impl.h"
#include "common/oauth2/util/constants.h"
#include "common/oauth2/util/util.h"

namespace opera {
namespace oauth2 {

// static
scoped_refptr<MigrationTokenRequest> MigrationTokenRequest::Create(
    GURL oauth1_base_url,
    const std::string& oauth1_client_id,
    const std::string& oauth1_client_secret,
    base::TimeDelta time_skew,
    const std::string& oauth1_token,
    const std::string& oauth1_token_secret,
    ScopeSet scopes,
    GURL oauth2_base_url,
    const std::string& oauth2_client_id,
    DeviceNameService* device_name_service,
    const std::string& session_id) {
  DCHECK(!oauth1_token.empty());
  DCHECK(!oauth1_token_secret.empty());
  DCHECK(!scopes.empty());

  return new MigrationTokenRequest(
      base::WrapUnique(new RequestVarsEncoderImpl),
      base::WrapUnique(new RequestVarsEncoderImpl), oauth1_base_url,
      oauth1_client_id, oauth1_client_secret, time_skew, oauth1_token,
      oauth1_token_secret, scopes, oauth2_base_url, oauth2_client_id,
      device_name_service, session_id);
}

MigrationTokenRequest::MigrationTokenRequest(
    std::unique_ptr<RequestVarsEncoder> post_vars_encoder,
    std::unique_ptr<RequestVarsEncoder> get_vars_encoder,
    GURL oauth1_base_url,
    const std::string& oauth1_client_id,
    const std::string& oauth1_client_secret,
    base::TimeDelta time_skew,
    const std::string& oauth1_token,
    const std::string& oauth1_token_secret,
    ScopeSet scopes,
    GURL oauth2_base_url,
    const std::string& oauth2_client_id,
    DeviceNameService* device_name_service,
    const std::string& session_id)
    : post_vars_encoder_(std::move(post_vars_encoder)),
      get_vars_encoder_(std::move(get_vars_encoder)),
      device_name_service_(device_name_service) {
  DCHECK(post_vars_encoder_);
  DCHECK(get_vars_encoder_);
  DCHECK(!oauth1_client_id.empty());
  DCHECK(!oauth1_client_secret.empty());
  DCHECK(!oauth1_token.empty());
  DCHECK(!oauth1_token_secret.empty());
  DCHECK(!scopes.empty());
  DCHECK(oauth1_base_url.is_valid());
  DCHECK(oauth2_base_url.is_valid());
  DCHECK(device_name_service_);

  if (!session_id.empty()) {
    get_vars_encoder_->AddVar(kSid, session_id);
  }

  oauth1_client_id_ = oauth1_client_id;
  oauth1_client_secret_ = oauth1_client_secret;
  oauth1_token_ = oauth1_token;
  oauth1_token_secret_ = oauth1_token_secret;
  oauth1_base_url_ = oauth1_base_url;
  oauth2_base_url_ = oauth2_base_url;
  time_skew_ = time_skew;

  const std::string encoded_scopes = scopes.encode();
  post_vars_encoder_->AddVar(kClientId, oauth2_client_id);
  post_vars_encoder_->AddVar(kScope, encoded_scopes);
  post_vars_encoder_->AddVar(kGrantType, kOAuth1Token);

  if (device_name_service_->HasDeviceNameChanged()) {
    post_vars_encoder_->AddVar(kDeviceName,
                               device_name_service_->GetCurrentDeviceName());
  }
}

MigrationTokenRequest::~MigrationTokenRequest() {}

std::string MigrationTokenRequest::GetContent() const {
  return post_vars_encoder_->EncodeFormEncoded();
}

std::string MigrationTokenRequest::GetPath() const {
  return "/oauth2/v1/token/";
}

NetworkResponseStatus MigrationTokenRequest::TryResponse(
    int http_code,
    const std::string& response_data) {
  DCHECK(device_name_service_);
  migration_token_response_.reset(new MigrationTokenResponse);
  auto response =
      migration_token_response_->TryResponse(http_code, response_data);
  if (200 == http_code && RS_OK == response &&
      post_vars_encoder_->HasVar(kDeviceName)) {
    device_name_service_->StoreDeviceName(
        post_vars_encoder_->GetVar(kDeviceName));
  }
  return response;
}

MigrationTokenResponse* MigrationTokenRequest::migration_token_response()
    const {
  return migration_token_response_.get();
}

net::URLFetcher::RequestType MigrationTokenRequest::GetHTTPRequestType() const {
  return net::URLFetcher::POST;
}

std::string MigrationTokenRequest::GetRequestContentType() const {
  return "application/x-www-form-urlencoded";
}

int MigrationTokenRequest::GetLoadFlags() const {
  return net::LOAD_DISABLE_CACHE | net::LOAD_DO_NOT_SAVE_COOKIES |
         net::LOAD_DO_NOT_SEND_COOKIES;
}

std::vector<std::string> MigrationTokenRequest::GetExtraRequestHeaders() const {
  const GURL migration_url(
      oauth2_base_url_.Resolve(GetPath()).Resolve("?" + GetQueryString()));
  OAuthRequestSigner::HttpMethod kMigrationHttpMethod(
      OAuthRequestSigner::POST_METHOD);
  const std::string kMigrationRealm = oauth1_base_url_.host();

  OAuthRequestSigner::Parameters parameters;
  parameters["opera_time_skew"] = base::Int64ToString(time_skew_.InSeconds());

  std::string result;
  const bool success = OAuthRequestSigner::SignAuthHeader(
      migration_url, parameters, OAuthRequestSigner::HMAC_SHA1_SIGNATURE,
      kMigrationHttpMethod, oauth1_client_id_, oauth1_client_secret_,
      oauth1_token_, oauth1_token_secret_, &result);
  DCHECK(success);

  // strlen("OAuth ") == 6
  result.insert(6, "realm=\"" + kMigrationRealm + "\", ");
  return std::vector<std::string>{"Authorization:" + result};
}

std::string MigrationTokenRequest::GetQueryString() const {
  return get_vars_encoder_->EncodeQueryString();
}

const std::string MigrationTokenRequest::GetDiagnosticDescription() const {
  std::string device_name;
  if (post_vars_encoder_->HasVar(kDeviceName)) {
    device_name = post_vars_encoder_->GetVar(kDeviceName);
  }
  std::string sid;
  if (get_vars_encoder_->HasVar(kSid)) {
    sid = get_vars_encoder_->GetVar(kSid);
  }
  return post_vars_encoder_->GetVar(kGrantType) + " " +
         post_vars_encoder_->GetVar(kScope) + " " + device_name + " " + sid;
}

RequestManagerUrlType MigrationTokenRequest::GetRequestManagerUrlType() const {
  return RMUT_OAUTH2;
}

}  // namespace oauth2
}  // namespace opera
