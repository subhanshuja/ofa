// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_NETWORK_NETWORK_REQUEST_MOCK_H_
#define COMMON_OAUTH2_NETWORK_NETWORK_REQUEST_MOCK_H_

#include "common/oauth2/network/network_request.h"

#include <string>
#include <vector>

#include "testing/gmock/include/gmock/gmock.h"

namespace opera {
namespace oauth2 {

class NetworkRequestMock : public NetworkRequest {
 public:
  NetworkRequestMock();

  MOCK_CONST_METHOD0(GetContent, std::string());
  MOCK_CONST_METHOD0(GetPath, std::string());
  MOCK_METHOD2(TryResponse, NetworkResponseStatus(int, const std::string&));
  MOCK_CONST_METHOD0(GetHTTPRequestType, net::URLFetcher::RequestType());
  MOCK_CONST_METHOD0(GetRequestContentType, std::string());
  MOCK_CONST_METHOD0(GetLoadFlags, int());
  MOCK_CONST_METHOD0(GetDiagnosticDescription, const std::string());
  MOCK_CONST_METHOD0(GetExtraRequestHeaders, std::vector<std::string>());
  MOCK_CONST_METHOD0(GetQueryString, std::string());
  MOCK_CONST_METHOD0(GetRequestManagerUrlType, RequestManagerUrlType());

 protected:
  ~NetworkRequestMock() override;
};

}  // namespace oauth2
}  // namespace opera

#endif  // COMMON_OAUTH2_NETWORK_NETWORK_REQUEST_MOCK_H_
