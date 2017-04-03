// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_NETWORK_NETWORK_REQUEST_H_
#define COMMON_OAUTH2_NETWORK_NETWORK_REQUEST_H_

#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "net/url_request/url_fetcher.h"

#include "common/oauth2/util/util.h"

namespace opera {
namespace oauth2 {

/**
* NetworkRequest implementations abstract OAuth2 network requests from
* the request data in order to allow pluggin them into the
* NetworkRequestManager.
*/
class NetworkRequest : public base::RefCounted<NetworkRequest> {
 public:
  /**
   * Request content body directly how it should be sent over the
   * wire.
   */
  virtual std::string GetContent() const = 0;

  /**
   * Request path. Note that the path will be appended to the base auth
   * service URL by the request manager.
   */
  virtual std::string GetPath() const = 0;

  /**
   * The network request manager will feed any response it gets from the remote
   * service to this method.
   * The specific OAuth2NetworkRequest implementation is supposed to decide if
   * the response is acceptable
   * and return a ResponseStatus that will determine the further fate of the
   * request in the request manager.
   */
  virtual NetworkResponseStatus TryResponse(
      int http_code,
      const std::string& response_data) = 0;

  /**
   * HTTP request type, most of the time a POST request.
   */
  virtual net::URLFetcher::RequestType GetHTTPRequestType() const = 0;

  /**
   * Encoding of the request content.
   */
  virtual std::string GetRequestContentType() const = 0;

  /**
   * Load flags for the URL fetcher.
   */
  virtual int GetLoadFlags() const = 0;

  /**
   * Request manager will append the headers to each request.
   * Note that the method will be run with each retry.
   */
  virtual std::vector<std::string> GetExtraRequestHeaders() const = 0;

  /**
   * Should return an encoded query string to append to the path.
   * Should not return the '?' character at the begninning.
   */
  virtual std::string GetQueryString() const = 0;

  virtual const std::string GetDiagnosticDescription() const = 0;

  /**
   * One of the base URL IDs that the request manager is configured for.
   */
  virtual RequestManagerUrlType GetRequestManagerUrlType() const = 0;

 protected:
  friend class base::RefCounted<NetworkRequest>;

  virtual ~NetworkRequest() {}
};
}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_NETWORK_NETWORK_REQUEST_H_
