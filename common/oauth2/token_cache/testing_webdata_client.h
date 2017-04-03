// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_TOKEN_CACHE_TESTING_WEBDATA_CLIENT_H_
#define COMMON_OAUTH2_TOKEN_CACHE_TESTING_WEBDATA_CLIENT_H_

#include <string>
#include <vector>

#include "common/oauth2/token_cache/webdata_client.h"

namespace opera {
namespace oauth2 {

class TestingWebdataClient : public WebdataClient {
 public:
  TestingWebdataClient();
  ~TestingWebdataClient() override;

  void ClearTokens() override;
  void LoadTokens(
      base::Callback<void(std::vector<scoped_refptr<const AuthToken>>)>
          callback) override;
  void SaveTokens(std::vector<scoped_refptr<const AuthToken>> tokens) override;

  std::vector<scoped_refptr<const AuthToken>> GetTestingTokens();
  void SetTestingTokens(std::vector<scoped_refptr<const AuthToken>> tokens);

  int clear_count() const { return clear_count_; }
  int load_count() const { return load_count_; }
  int save_count() const { return save_count_; }

 private:
  int clear_count_;
  int load_count_;
  int save_count_;
  std::vector<scoped_refptr<const AuthToken>> tokens_;
};
}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_TOKEN_CACHE_TESTING_WEBDATA_CLIENT_H_
