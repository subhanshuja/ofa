// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/network/access_token_request.h"

#include <string>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/values.h"

#include "common/oauth2/device_name/device_name_service.h"
#include "common/oauth2/network/access_token_response.h"
#include "common/oauth2/network/request_vars_encoder_impl.h"
#include "common/oauth2/util/constants.h"
#include "common/oauth2/util/util.h"

namespace opera {
namespace oauth2 {

// static
scoped_refptr<AccessTokenRequest> AccessTokenRequest::CreateWithAuthTokenGrant(
    const std::string& auth_token,
    ScopeSet scopes,
    const std::string& client_id,
    DeviceNameService* device_name_service,
    const std::string& session_id) {
  return new AccessTokenRequest(base::WrapUnique(new RequestVarsEncoderImpl),
                                base::WrapUnique(new RequestVarsEncoderImpl),
                                GT_AUTH_TOKEN, auth_token, scopes, client_id,
                                device_name_service, session_id);
}

// static
scoped_refptr<AccessTokenRequest>
AccessTokenRequest::CreateWithRefreshTokenGrant(
    const std::string& refresh_token,
    ScopeSet scopes,
    const std::string& client_id,
    DeviceNameService* device_name_service,
    const std::string& session_id) {
  return new AccessTokenRequest(base::WrapUnique(new RequestVarsEncoderImpl),
                                base::WrapUnique(new RequestVarsEncoderImpl),
                                GT_REFRESH_TOKEN, refresh_token, scopes,
                                client_id, device_name_service, session_id);
}

AccessTokenRequest::AccessTokenRequest(
    std::unique_ptr<RequestVarsEncoder> post_var_encoder,
    std::unique_ptr<RequestVarsEncoder> get_var_encoder,
    GrantType grant_type,
    const std::string& grant,
    ScopeSet scopes,
    const std::string& client_id,
    DeviceNameService* device_name_service,
    const std::string& session_id)
    : requested_scopes_(scopes),
      post_var_encoder_(std::move(post_var_encoder)),
      get_var_encoder_(std::move(get_var_encoder)),
      device_name_service_(device_name_service) {
  DCHECK(post_var_encoder_);
  DCHECK(get_var_encoder_);
  DCHECK(!grant.empty());
  DCHECK(!scopes.empty());
  DCHECK(device_name_service_);

  if (!session_id.empty()) {
    get_var_encoder_->AddVar(kSid, session_id);
  }

  if (device_name_service_->HasDeviceNameChanged()) {
    post_var_encoder_->AddVar(kDeviceName,
                              device_name_service_->GetCurrentDeviceName());
  }

  const std::string encoded_scopes = scopes.encode();
  post_var_encoder_->AddVar(kClientId, client_id);
  post_var_encoder_->AddVar(kScope, encoded_scopes);

  grant_type_ = grant_type;
  switch (grant_type) {
    case GT_REFRESH_TOKEN:
      post_var_encoder_->AddVar(kGrantType, kRefreshToken);
      post_var_encoder_->AddVar(kRefreshToken, grant);
      break;
    case GT_AUTH_TOKEN:
      post_var_encoder_->AddVar(kGrantType, kAuthToken);
      post_var_encoder_->AddVar(kAuthToken, grant);
      break;
    default:
      NOTREACHED();
  }
}

AccessTokenRequest::~AccessTokenRequest() {}

std::string AccessTokenRequest::GetContent() const {
  return post_var_encoder_->EncodeFormEncoded();
}

std::string AccessTokenRequest::GetPath() const {
  return "/oauth2/v1/token/";
}

NetworkResponseStatus AccessTokenRequest::TryResponse(
    int http_code,
    const std::string& response_data) {
  DCHECK(device_name_service_);
  access_token_response_.reset(new AccessTokenResponse);

  bool expecting_refresh_token = false;
  switch (grant_type_) {
    case GT_AUTH_TOKEN:
      expecting_refresh_token = true;
      break;
    case GT_REFRESH_TOKEN:
      expecting_refresh_token = false;
      break;
    default:
      NOTREACHED();
  }

  const auto status = access_token_response_->TryResponse(
      http_code, response_data, expecting_refresh_token);

  if (200 == http_code && RS_OK == status &&
      post_var_encoder_->HasVar(kDeviceName)) {
    device_name_service_->StoreDeviceName(
        post_var_encoder_->GetVar(kDeviceName));
  }
  return status;
}

ScopeSet AccessTokenRequest::requested_scopes() const {
  return requested_scopes_;
}

AccessTokenResponse* AccessTokenRequest::access_token_response() const {
  return access_token_response_.get();
}

net::URLFetcher::RequestType AccessTokenRequest::GetHTTPRequestType() const {
  return net::URLFetcher::POST;
}

std::string AccessTokenRequest::GetRequestContentType() const {
  return "application/x-www-form-urlencoded";
}

int AccessTokenRequest::GetLoadFlags() const {
  return net::LOAD_DISABLE_CACHE | net::LOAD_DO_NOT_SAVE_COOKIES |
         net::LOAD_DO_NOT_SEND_COOKIES;
}

std::vector<std::string> AccessTokenRequest::GetExtraRequestHeaders() const {
  return std::vector<std::string>{};
}

std::string AccessTokenRequest::GetQueryString() const {
  return get_var_encoder_->EncodeQueryString();
}

const std::string AccessTokenRequest::GetDiagnosticDescription() const {
  std::string device_name;
  if (post_var_encoder_->HasVar(kDeviceName)) {
    device_name = post_var_encoder_->GetVar(kDeviceName);
  }
  std::string sid;
  if (get_var_encoder_->HasVar(kSid)) {
    sid = get_var_encoder_->GetVar(kSid);
  }
  return post_var_encoder_->GetVar(kGrantType) + " " +
         post_var_encoder_->GetVar(kScope) + " " + device_name + " " + sid;
}

RequestManagerUrlType AccessTokenRequest::GetRequestManagerUrlType() const {
  return RMUT_OAUTH2;
}

}  // namespace oauth2
}  // namespace opera
