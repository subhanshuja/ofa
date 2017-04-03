// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "components/sync/engine_impl/net/oauth_signer.h"

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "google_apis/gaia/oauth_request_signer.h"
#include "url/gurl.h"

namespace sync_oauth_signer {

SignParams::SignParams() {}

SignParams::SignParams(const std::string& realm,
                       const std::string& client_key,
                       const std::string& client_secret,
                       const std::string& token,
                       const std::string& secret,
                       int64_t time_skew)
    : realm_(realm),
      client_key_(client_key),
      client_secret_(client_secret),
      token_(token),
      secret_(secret),
      time_skew_(time_skew) {
}

SignParams::SignParams(const SignParams&) = default;

SignParams::~SignParams() {
}

std::string SignParams::GetSignedAuthHeader(
    const std::string& request_base_url) const {
  DCHECK(IsValid());
  OAuthRequestSigner::Parameters parameters;
  std::string result;
  parameters["opera_time_skew"] = base::Int64ToString(time_skew_);

  const bool success = OAuthRequestSigner::SignAuthHeader(
      GURL(request_base_url),
      parameters,
      OAuthRequestSigner::HMAC_SHA1_SIGNATURE,
      OAuthRequestSigner::POST_METHOD,
      client_key_,
      client_secret_,
      token_,
      secret_,
      &result);
  DCHECK(success);

  // strlen("OAuth ") == 6
  result.insert(6, "realm=\"" + realm_ + "\", ");
  return result;
}

bool SignParams::IsValid() const {
  return !realm_.empty() && !client_key_.empty() && !client_secret_.empty();
}

void SignParams::UpdateToken(const std::string& new_token,
                             const std::string& new_token_secret) {
  token_ = new_token;
  secret_ = new_token_secret;
}

bool SignParams::UpdateTimeSkew(int64_t time_skew) {
  if (time_skew != time_skew_) {
    time_skew_ = time_skew;
    return true;
  }
  return false;
}

void SignParams::ClearToken() {
  UpdateToken("", "");
}

}  // namespace sync_oauth_signer
