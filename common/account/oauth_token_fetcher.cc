// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/account/oauth_token_fetcher.h"

#include "common/account/account_auth_data.h"

namespace opera {

OAuthTokenFetcher::OAuthTokenFetcher() {
}

OAuthTokenFetcher::~OAuthTokenFetcher() {
}

void OAuthTokenFetcher::SetUserCredentials(const std::string& user_name,
                                           const std::string& password) {
  user_name_ = user_name;
  password_ = password;
}

void OAuthTokenFetcher::RequestAuthData(
    opera_sync::OperaAuthProblem problem,
    const RequestAuthDataSuccessCallback& success_callback,
    const RequestAuthDataFailureCallback& failure_callback) {
  auth_problem_ = problem;
  success_callback_ = success_callback;
  failure_callback_ = failure_callback;
  Start();
}

}  // namespace opera
