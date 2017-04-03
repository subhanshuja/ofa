// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/network/revoke_token_request.h"

#include <string>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "net/base/load_flags.h"

#include "common/oauth2/network/request_vars_encoder_impl.h"
#include "common/oauth2/network/revoke_token_response.h"
#include "common/oauth2/util/constants.h"
#include "common/oauth2/util/util.h"

namespace opera {
namespace oauth2 {

// static
scoped_refptr<RevokeTokenRequest> RevokeTokenRequest::Create(
    TokenType token_type,
    const std::string& token,
    const std::string& client_id,
    const std::string& session_id) {
  return new RevokeTokenRequest(base::WrapUnique(new RequestVarsEncoderImpl),
                                base::WrapUnique(new RequestVarsEncoderImpl),
                                token_type, token, client_id, session_id);
}

RevokeTokenRequest::RevokeTokenRequest(
    std::unique_ptr<RequestVarsEncoder> post_vars_encoder,
    std::unique_ptr<RequestVarsEncoder> get_vars_encoder,
    TokenType token_type,
    const std::string& token,
    const std::string& client_id,
    const std::string& session_id)
    : post_vars_encoder_(std::move(post_vars_encoder)),
      get_vars_encoder_(std::move(get_vars_encoder)) {
  DCHECK(post_vars_encoder_);
  DCHECK(get_vars_encoder_);
  DCHECK(!token.empty());
  DCHECK(!client_id.empty());

  if (!session_id.empty()) {
    get_vars_encoder_->AddVar(kSid, session_id);
  }

  post_vars_encoder_->AddVar(kToken, token);
  post_vars_encoder_->AddVar(kClientId, client_id);

  std::string token_type_hint;
  if (token_type == TT_REFRESH_TOKEN) {
    token_type_hint = kRefreshToken;
  } else if (token_type == TT_ACCESS_TOKEN) {
    token_type_hint = kAccessToken;
  } else {
    NOTREACHED();
  }
  post_vars_encoder_->AddVar(kTokenTypeHint, token_type_hint);
}

RevokeTokenRequest::~RevokeTokenRequest() {}

std::string RevokeTokenRequest::GetContent() const {
  return post_vars_encoder_->EncodeFormEncoded();
}

std::string RevokeTokenRequest::GetPath() const {
  return "/oauth2/v1/revoketoken/";
}

NetworkResponseStatus RevokeTokenRequest::TryResponse(
    int http_code,
    const std::string& response_data) {
  revoke_token_response_.reset(new RevokeTokenResponse);
  return revoke_token_response_->TryResponse(http_code, response_data);
}

RevokeTokenResponse* RevokeTokenRequest::revoke_token_response() const {
  return revoke_token_response_.get();
}

net::URLFetcher::RequestType RevokeTokenRequest::GetHTTPRequestType() const {
  return net::URLFetcher::POST;
}

std::string RevokeTokenRequest::GetRequestContentType() const {
  return "application/x-www-form-urlencoded";
}

int RevokeTokenRequest::GetLoadFlags() const {
  return net::LOAD_DISABLE_CACHE | net::LOAD_DO_NOT_SAVE_COOKIES |
         net::LOAD_DO_NOT_SEND_COOKIES;
}

std::vector<std::string> RevokeTokenRequest::GetExtraRequestHeaders() const {
  return std::vector<std::string>{};
}

std::string RevokeTokenRequest::GetQueryString() const {
  return get_vars_encoder_->EncodeQueryString();
}

const std::string RevokeTokenRequest::GetDiagnosticDescription() const {
  std::string sid;
  if (get_vars_encoder_->HasVar(kSid)) {
    sid = get_vars_encoder_->GetVar(kSid);
  }
  return post_vars_encoder_->GetVar(kTokenTypeHint) + " " + sid;
}

RequestManagerUrlType RevokeTokenRequest::GetRequestManagerUrlType() const {
  return RMUT_OAUTH2;
}

}  // namespace oauth2
}  // namespace opera
