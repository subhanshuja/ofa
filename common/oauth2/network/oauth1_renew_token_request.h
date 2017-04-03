// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_NETWORK_OAUTH1_RENEW_TOKEN_REQUEST_H_
#define COMMON_OAUTH2_NETWORK_OAUTH1_RENEW_TOKEN_REQUEST_H_

#include <memory>
#include <string>
#include <vector>

#include "net/base/load_flags.h"

#include "common/oauth2/network/network_request.h"
#include "common/oauth2/network/request_vars_encoder.h"
#include "common/oauth2/util/scope_set.h"

namespace opera {
namespace oauth2 {

class OAuth1RenewTokenResponse;

class OAuth1RenewTokenRequest : public NetworkRequest {
 public:
  static scoped_refptr<OAuth1RenewTokenRequest> Create(
      const std::string& oauth1_auth_service_id,
      const std::string& oauth1_old_token,
      const std::string& oauth1_client_id,
      const std::string& oauth1_client_secret);

  std::string GetContent() const override;
  std::string GetPath() const override;
  NetworkResponseStatus TryResponse(int http_code,
                                    const std::string& response_data) override;

  OAuth1RenewTokenResponse* oauth1_renew_token_response() const;

  net::URLFetcher::RequestType GetHTTPRequestType() const override;

  std::string GetRequestContentType() const override;

  int GetLoadFlags() const override;

  std::vector<std::string> GetExtraRequestHeaders() const override;

  std::string GetQueryString() const override;

  const std::string GetDiagnosticDescription() const override;

  RequestManagerUrlType GetRequestManagerUrlType() const override;

 protected:
  OAuth1RenewTokenRequest(std::unique_ptr<RequestVarsEncoder> encoder,
                          const std::string& oauth1_auth_service_id,
                          const std::string& oauth1_old_token,
                          const std::string& oauth1_client_id,
                          const std::string& oauth1_client_secret);

  ~OAuth1RenewTokenRequest() override;

  std::unique_ptr<OAuth1RenewTokenResponse> oauth1_renew_token_response_;
  std::unique_ptr<RequestVarsEncoder> encoder_;

 private:
  DISALLOW_COPY_AND_ASSIGN(OAuth1RenewTokenRequest);
};
}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_NETWORK_OAUTH1_RENEW_TOKEN_REQUEST_H_
