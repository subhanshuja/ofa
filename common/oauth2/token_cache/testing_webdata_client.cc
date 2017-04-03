// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/token_cache/testing_webdata_client.h"

namespace opera {
namespace oauth2 {

TestingWebdataClient::TestingWebdataClient()
    : clear_count_(0), load_count_(0), save_count_(0) {}

TestingWebdataClient::~TestingWebdataClient() {}

void TestingWebdataClient::ClearTokens() {
  clear_count_++;
}

void TestingWebdataClient::LoadTokens(
    base::Callback<void(std::vector<scoped_refptr<const AuthToken>>)>
        callback) {
  load_count_++;
  callback.Run(tokens_);
}

void TestingWebdataClient::SaveTokens(
    std::vector<scoped_refptr<const AuthToken>> tokens) {
  save_count_++;
  tokens_ = tokens;
}

std::vector<scoped_refptr<const AuthToken>>
TestingWebdataClient::GetTestingTokens() {
  return tokens_;
}

void TestingWebdataClient::SetTestingTokens(
    std::vector<scoped_refptr<const AuthToken>> tokens) {
  tokens_ = tokens;
}

}  // namespace oauth2
}  // namespace opera
