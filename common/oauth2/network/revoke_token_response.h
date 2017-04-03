// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_NETWORK_REVOKE_TOKEN_RESPONSE_H_
#define COMMON_OAUTH2_NETWORK_REVOKE_TOKEN_RESPONSE_H_

#include "common/oauth2/util/util.h"

namespace opera {
namespace oauth2 {

class RevokeTokenResponse {
 public:
  RevokeTokenResponse();
  ~RevokeTokenResponse();

  NetworkResponseStatus TryResponse(int http_code,
                                    const std::string& response_data);

  OperaAuthError auth_error() const { return auth_error_; }
  std::string error_message() const { return error_message_; }

 protected:
  NetworkResponseStatus ParseForHTTP200(const std::string& response_data);
  NetworkResponseStatus ParseForHTTP400(const std::string& response_data);
  NetworkResponseStatus ParseForHTTP401(const std::string& response_data);

  OperaAuthError auth_error_;
  std::string error_message_;
};
}  // namespace oauth2
}  // namespace opera

#endif  // COMMON_OAUTH2_NETWORK_REVOKE_TOKEN_RESPONSE_H_
