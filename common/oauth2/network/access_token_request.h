// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_NETWORK_ACCESS_TOKEN_REQUEST_H_
#define COMMON_OAUTH2_NETWORK_ACCESS_TOKEN_REQUEST_H_

#include <memory>
#include <string>
#include <vector>

#include "net/base/load_flags.h"

#include "common/oauth2/network/network_request.h"
#include "common/oauth2/network/request_vars_encoder.h"
#include "common/oauth2/util/scope_set.h"

namespace opera {
namespace oauth2 {

class AccessTokenResponse;
class DeviceNameService;

class AccessTokenRequest : public NetworkRequest {
 public:
  enum GrantType { GT_REFRESH_TOKEN, GT_AUTH_TOKEN };

  static scoped_refptr<AccessTokenRequest> CreateWithAuthTokenGrant(
      const std::string& auth_token,
      ScopeSet scopes,
      const std::string& client_id,
      DeviceNameService* device_name_service,
      const std::string& session_id);

  static scoped_refptr<AccessTokenRequest> CreateWithRefreshTokenGrant(
      const std::string& refresh_token,
      ScopeSet scopes,
      const std::string& client_id,
      DeviceNameService* device_name_service,
      const std::string& session_id);

  std::string GetContent() const override;
  std::string GetPath() const override;
  NetworkResponseStatus TryResponse(int http_code,
                                    const std::string& response_data) override;

  ScopeSet requested_scopes() const;

  AccessTokenResponse* access_token_response() const;

  net::URLFetcher::RequestType GetHTTPRequestType() const override;

  std::string GetRequestContentType() const override;

  int GetLoadFlags() const override;

  std::vector<std::string> GetExtraRequestHeaders() const override;

  std::string GetQueryString() const override;

  const std::string GetDiagnosticDescription() const override;

  RequestManagerUrlType GetRequestManagerUrlType() const override;

 protected:
  AccessTokenRequest(std::unique_ptr<RequestVarsEncoder> post_var_encoder,
                     std::unique_ptr<RequestVarsEncoder> get_var_encoder,
                     GrantType grant_type,
                     const std::string& grant,
                     ScopeSet scopes,
                     const std::string& client_id,
                     DeviceNameService* device_name_service,
                     const std::string& session_id);

  ~AccessTokenRequest() override;

  std::unique_ptr<AccessTokenResponse> access_token_response_;
  ScopeSet requested_scopes_;

  std::unique_ptr<RequestVarsEncoder> post_var_encoder_;
  std::unique_ptr<RequestVarsEncoder> get_var_encoder_;
  DeviceNameService* device_name_service_;
  GrantType grant_type_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AccessTokenRequest);
};
}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_NETWORK_ACCESS_TOKEN_REQUEST_H_
