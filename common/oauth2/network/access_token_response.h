// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_NETWORK_ACCESS_TOKEN_RESPONSE_H_
#define COMMON_OAUTH2_NETWORK_ACCESS_TOKEN_RESPONSE_H_

#include "base/time/time.h"

#include "common/oauth2/util/scope_set.h"
#include "common/oauth2/util/util.h"

namespace opera {
namespace oauth2 {

class AccessTokenResponse {
 public:
  AccessTokenResponse();
  ~AccessTokenResponse();

  NetworkResponseStatus TryResponse(int http_code,
                                    const std::string& response_data,
                                    bool expecting_refresh_token);

  OperaAuthError auth_error() const { return auth_error_; }
  std::string error_decription() const { return error_description_; }
  std::string access_token() const { return access_token_; }
  std::string refresh_token() const { return refresh_token_; }
  base::TimeDelta expires_in() const { return expires_in_; }
  ScopeSet scopes() const { return scopes_; }
  std::string user_id() const { return user_id_; }

 protected:
  NetworkResponseStatus ParseForHTTP200(const std::string& response_data,
                                        bool expecting_refresh_token);
  NetworkResponseStatus ParseForHTTP400(const std::string& response_data);
  NetworkResponseStatus ParseForHTTP401(const std::string& response_data);

  OperaAuthError auth_error_;
  std::string error_description_;
  std::string access_token_;
  std::string refresh_token_;
  base::TimeDelta expires_in_;
  ScopeSet scopes_;
  std::string user_id_;
};

}  // namespace oauth2
}  // namespace opera

#endif  // COMMON_OAUTH2_NETWORK_ACCESS_TOKEN_RESPONSE_H_
