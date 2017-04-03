// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_NETWORK_NETWORK_REQUEST_MANAGER_MOCK_H_
#define COMMON_OAUTH2_NETWORK_NETWORK_REQUEST_MANAGER_MOCK_H_

#include "testing/gmock/include/gmock/gmock.h"

#include "common/oauth2/network/network_request_manager.h"

namespace opera {
namespace oauth2 {

class NetworkRequestManagerMock : public NetworkRequestManager {
 public:
  NetworkRequestManagerMock();
  ~NetworkRequestManagerMock() override;

  MOCK_METHOD1(OnURLFetchComplete, void(const net::URLFetcher*));
  MOCK_METHOD2(StartRequest,
               void(scoped_refptr<NetworkRequest>, base::WeakPtr<Consumer>));
  MOCK_METHOD0(CancelAllRequests, void());
};
}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_NETWORK_NETWORK_REQUEST_MANAGER_MOCK_H_
