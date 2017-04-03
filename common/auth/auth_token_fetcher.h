// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2017 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#ifndef COMMON_AUTH_AUTH_TOKEN_FETCHER_H_
#define COMMON_AUTH_AUTH_TOKEN_FETCHER_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "net/url_request/url_fetcher_delegate.h"

namespace net {
class URLFetcher;
class URLRequestContextGetter;
}

namespace opera {

class AuthTokenConsumer;

// Fetches an OAuth2 auth token from the AUTH service. Auth tokens can later be
// used to retrieve an access and refresh token.
//
// AuthTokenFetcher should only be used on the same thread as its consumer
class AuthTokenFetcher : public net::URLFetcherDelegate {
 public:
  explicit AuthTokenFetcher(net::URLRequestContextGetter* getter);
  ~AuthTokenFetcher() override;

  // Start a request to fetch an auth token for the user identified by
  // |username| and |password|. |consumer| will be invoked on the same thread
  // used when calling this method.
  void RequestAuthTokenForOAuth2(
      const std::string& username,
      const std::string& password,
      const base::WeakPtr<AuthTokenConsumer>& consumer);

  // net::URLFetcherDelegate implementation
  void OnURLFetchComplete(const net::URLFetcher* source);

 private:
  void HandleInitialGET(const net::URLFetcher* source);
  void HandleAuthenticationResponse(const net::URLFetcher* source);
  void RequestAuthTokenWithCSRFToken(const std::string& csrf_token);

  enum State {
    STATE_IDLE,
    STATE_FETCH_CSRF_TOKEN,
    STATE_FETCH_AUTH_TOKEN,
  };

  State state_;

  net::URLRequestContextGetter* const request_context_getter_;
  std::unique_ptr<net::URLFetcher> fetcher_;  // Only set during requests

  // ID for URLFetcher used during testing
  int url_fetcher_id_;

  std::string username_;
  std::string password_;

  base::WeakPtr<AuthTokenConsumer> consumer_;

  DISALLOW_COPY_AND_ASSIGN(AuthTokenFetcher);
};

}  // namespace opera

#endif  // COMMON_AUTH_AUTH_TOKEN_FETCHER_H_
