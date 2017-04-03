// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/network/oauth1_renew_token_request.h"

#include <memory>
#include <string>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/sha1.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"

#include "common/oauth2/network/oauth1_renew_token_response.h"
#include "common/oauth2/network/request_vars_encoder_impl.h"
#include "common/oauth2/util/constants.h"
#include "common/oauth2/util/util.h"

namespace opera {
namespace oauth2 {

// static
scoped_refptr<OAuth1RenewTokenRequest> OAuth1RenewTokenRequest::Create(
    const std::string& oauth1_auth_service_id,
    const std::string& oauth1_old_token,
    const std::string& oauth1_client_id,
    const std::string& oauth1_client_secret) {
  return new OAuth1RenewTokenRequest(
      base::WrapUnique(new RequestVarsEncoderImpl), oauth1_auth_service_id,
      oauth1_old_token, oauth1_client_id, oauth1_client_secret);
}

OAuth1RenewTokenRequest::OAuth1RenewTokenRequest(
    std::unique_ptr<RequestVarsEncoder> encoder,
    const std::string& oauth1_auth_service_id,
    const std::string& oauth1_old_token,
    const std::string& oauth1_client_id,
    const std::string& oauth1_client_secret)
    : encoder_(std::move(encoder)) {
  DCHECK(!oauth1_auth_service_id.empty());
  DCHECK(!oauth1_old_token.empty());
  DCHECK(!oauth1_client_id.empty());
  DCHECK(!oauth1_client_secret.empty());
  const char kSignatureBaseStringFormat[] =
      "consumer_key=%s&old_token=%s&service=%sX%s";

  const std::string signature_base_string = base::StringPrintf(
      kSignatureBaseStringFormat, oauth1_client_id.c_str(),
      oauth1_old_token.c_str(), oauth1_auth_service_id.c_str(),
      oauth1_client_secret.c_str());
  const std::string raw_signature = base::SHA1HashString(signature_base_string);
  const std::string signature = base::ToLowerASCII(
      base::HexEncode(raw_signature.data(), raw_signature.size()));

  encoder_->AddVar(kSignature, signature);

  encoder_->AddVar(kConsumerKey, oauth1_client_id);
  encoder_->AddVar(kService, oauth1_auth_service_id);
  encoder_->AddVar(kOldToken, oauth1_old_token);
}

OAuth1RenewTokenRequest::~OAuth1RenewTokenRequest() {}

std::string OAuth1RenewTokenRequest::GetContent() const {
  return std::string();
}

std::string OAuth1RenewTokenRequest::GetPath() const {
  return "/account/access-token/renewal/";
}

NetworkResponseStatus OAuth1RenewTokenRequest::TryResponse(
    int http_code,
    const std::string& response_data) {
  oauth1_renew_token_response_.reset(new OAuth1RenewTokenResponse);
  return oauth1_renew_token_response_->TryResponse(http_code, response_data);
}

OAuth1RenewTokenResponse* OAuth1RenewTokenRequest::oauth1_renew_token_response()
    const {
  return oauth1_renew_token_response_.get();
}

net::URLFetcher::RequestType OAuth1RenewTokenRequest::GetHTTPRequestType()
    const {
  return net::URLFetcher::GET;
}

std::string OAuth1RenewTokenRequest::GetRequestContentType() const {
  return "";
}

int OAuth1RenewTokenRequest::GetLoadFlags() const {
  return net::LOAD_DISABLE_CACHE | net::LOAD_DO_NOT_SAVE_COOKIES |
         net::LOAD_DO_NOT_SEND_COOKIES;
}

std::vector<std::string> OAuth1RenewTokenRequest::GetExtraRequestHeaders()
    const {
  return std::vector<std::string>{};
}

std::string OAuth1RenewTokenRequest::GetQueryString() const {
  return encoder_->EncodeQueryString();
}

const std::string OAuth1RenewTokenRequest::GetDiagnosticDescription() const {
  return std::string();
}

RequestManagerUrlType OAuth1RenewTokenRequest::GetRequestManagerUrlType()
    const {
  return RMUT_OAUTH1;
}

}  // namespace oauth2
}  // namespace opera
