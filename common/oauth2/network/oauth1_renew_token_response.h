// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_NETWORK_OAUTH1_RENEW_TOKEN_RESPONSE_H_
#define COMMON_OAUTH2_NETWORK_OAUTH1_RENEW_TOKEN_RESPONSE_H_

#include <string>

#include "base/time/time.h"

#include "common/oauth2/util/scope_set.h"
#include "common/oauth2/util/util.h"

namespace opera {
namespace oauth2 {

class OAuth1RenewTokenResponse {
 public:
  OAuth1RenewTokenResponse();
  ~OAuth1RenewTokenResponse();

  NetworkResponseStatus TryResponse(int http_code,
                                    const std::string& response_data);

  int oauth1_error_code() const;
  std::string oauth1_error_message() const;
  std::string oauth1_auth_token() const;
  std::string oauth1_auth_token_secret() const;

 protected:
  NetworkResponseStatus ParseForHTTP200(const std::string& response_data);

  int oauth1_error_code_;
  std::string oauth1_error_message_;
  std::string oauth1_auth_token_;
  std::string oauth1_auth_token_secret_;
};

}  // namespace oauth2
}  // namespace opera

#endif  // COMMON_OAUTH2_NETWORK_OAUTH1_RENEW_TOKEN_RESPONSE_H_
