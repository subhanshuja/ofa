// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_NETWORK_NETWORK_REQUEST_MANAGER_H_
#define COMMON_OAUTH2_NETWORK_NETWORK_REQUEST_MANAGER_H_

#include <string>

#include "base/memory/weak_ptr.h"

#include "common/oauth2/network/network_request.h"
#include "common/oauth2/util/util.h"

namespace opera {
namespace oauth2 {

class NetworkRequestManager {
 public:
  class Consumer {
   public:
    virtual ~Consumer() {}
    /**
     * A callback that the consumer will get once the connection manager is
     * finally done with the request.
     * This can currently happen in the following cases:
     * 1. The network connection was successfully completed: RS_OK;
     * 2. The security settings disallow even starting the connection:
     *    RS_INSECURE_CONNECTION_FORBIDDEN.
     *
     * Note that RS_HTTP_PROBLEM, RS_PARSE_PROBLEM and RS_THROTTLED, as well
     * as network connectivity problems will schedule a retry for the request
     * implicitly.
     */
    virtual void OnNetworkRequestFinished(
        scoped_refptr<NetworkRequest> request,
        NetworkResponseStatus response_status) = 0;

    virtual std::string GetConsumerName() const = 0;
  };

  virtual ~NetworkRequestManager() {}

  virtual void StartRequest(scoped_refptr<NetworkRequest> request,
                            base::WeakPtr<Consumer> consumer) = 0;

  virtual void CancelAllRequests() = 0;
};
}  // namespace oauth2
}  // namespace opera

#endif  // COMMON_OAUTH2_NETWORK_NETWORK_REQUEST_MANAGER_H_
