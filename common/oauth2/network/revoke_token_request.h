// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_OAUTH_NETWORK_REVOKE_REFRESH_REQUEST_H_
#define COMMON_OAUTH2_OAUTH_NETWORK_REVOKE_REFRESH_REQUEST_H_

#include <string>
#include <vector>

#include "common/oauth2/network/network_request.h"
#include "common/oauth2/network/request_vars_encoder.h"
#include "common/oauth2/util/util.h"

namespace opera {
namespace oauth2 {

class RevokeTokenResponse;

class RevokeTokenRequest : public NetworkRequest {
 public:
  enum TokenType { TT_REFRESH_TOKEN, TT_ACCESS_TOKEN };

  static scoped_refptr<RevokeTokenRequest> Create(
      TokenType token_type,
      const std::string& token,
      const std::string& client_id,
      const std::string& session_id);

  std::string GetContent() const override;
  std::string GetPath() const override;
  NetworkResponseStatus TryResponse(int http_code,
                                    const std::string& response_data) override;

  RevokeTokenResponse* revoke_token_response() const;

  net::URLFetcher::RequestType GetHTTPRequestType() const override;

  std::string GetRequestContentType() const override;

  int GetLoadFlags() const override;

  std::vector<std::string> GetExtraRequestHeaders() const override;

  std::string GetQueryString() const override;

  const std::string GetDiagnosticDescription() const override;

  RequestManagerUrlType GetRequestManagerUrlType() const override;

 protected:
  RevokeTokenRequest(std::unique_ptr<RequestVarsEncoder> post_vars_encoder,
                     std::unique_ptr<RequestVarsEncoder> get_vars_encoder,
                     TokenType token_type,
                     const std::string& token,
                     const std::string& client_id,
                     const std::string& session_id);

  ~RevokeTokenRequest() override;

  std::unique_ptr<RevokeTokenResponse> revoke_token_response_;
  std::unique_ptr<RequestVarsEncoder> post_vars_encoder_;
  std::unique_ptr<RequestVarsEncoder> get_vars_encoder_;

 private:
  DISALLOW_COPY_AND_ASSIGN(RevokeTokenRequest);
};
}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_OAUTH_NETWORK_REVOKE_REFRESH_REQUEST_H_
