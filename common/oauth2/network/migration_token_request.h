// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_NETWORK_MIGRATION_TOKEN_REQUEST_H_
#define COMMON_OAUTH2_NETWORK_MIGRATION_TOKEN_REQUEST_H_

#include <memory>
#include <string>
#include <vector>

#include "base/time/time.h"

#include "common/oauth2/device_name/device_name_service.h"
#include "common/oauth2/network/network_request.h"
#include "common/oauth2/network/request_vars_encoder.h"
#include "common/oauth2/util/scope_set.h"

namespace opera {
namespace oauth2 {

class MigrationTokenResponse;

class MigrationTokenRequest : public NetworkRequest {
 public:
  static scoped_refptr<MigrationTokenRequest> Create(
      GURL oauth1_base_url,
      const std::string& oauth1_client_id,
      const std::string& oauth1_client_secret,
      base::TimeDelta time_skew,
      const std::string& oauth1_token,
      const std::string& oauth1_token_secret,
      ScopeSet scopes,
      GURL oauth2_base_url,
      const std::string& oauth2_client_id,
      DeviceNameService* device_name_service,
      const std::string& session_id);

  std::string GetContent() const override;
  std::string GetPath() const override;
  NetworkResponseStatus TryResponse(int http_code,
                                    const std::string& response_data) override;

  MigrationTokenResponse* migration_token_response() const;

  net::URLFetcher::RequestType GetHTTPRequestType() const override;

  std::string GetRequestContentType() const override;

  int GetLoadFlags() const override;

  std::vector<std::string> GetExtraRequestHeaders() const override;

  std::string GetQueryString() const override;

  const std::string GetDiagnosticDescription() const override;

  RequestManagerUrlType GetRequestManagerUrlType() const override;

 protected:
  MigrationTokenRequest(std::unique_ptr<RequestVarsEncoder> post_vars_encoder,
                        std::unique_ptr<RequestVarsEncoder> get_vars_encoder,
                        GURL oauth1_base_url,
                        const std::string& oauth1_client_id,
                        const std::string& oauth1_client_secret,
                        base::TimeDelta time_skew,
                        const std::string& oauth1_token,
                        const std::string& oauth1_token_secret,
                        ScopeSet scopes,
                        GURL oauth2_base_url,
                        const std::string& oauth2_client_id,
                        DeviceNameService* device_name_service,
                        const std::string& session_id);

  ~MigrationTokenRequest() override;

  base::TimeDelta time_skew_;
  std::string oauth1_client_id_;
  std::string oauth1_client_secret_;
  std::string oauth1_token_;
  std::string oauth1_token_secret_;
  GURL oauth1_base_url_;
  GURL oauth2_base_url_;

  std::unique_ptr<MigrationTokenResponse> migration_token_response_;
  std::unique_ptr<RequestVarsEncoder> post_vars_encoder_;
  std::unique_ptr<RequestVarsEncoder> get_vars_encoder_;
  DeviceNameService* device_name_service_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MigrationTokenRequest);
};
}  // namespace oauth2
}  // namespace opera

#endif  // COMMON_OAUTH2_NETWORK_MIGRATION_TOKEN_REQUEST_H_
