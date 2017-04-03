// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_TOKEN_CACHE_WEBDATA_CLIENT_H_
#define COMMON_OAUTH2_TOKEN_CACHE_WEBDATA_CLIENT_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "components/keyed_service/core/keyed_service.h"

#include "common/oauth2/util/token.h"

namespace opera {
namespace oauth2 {

class WebdataClient : public KeyedService {
 public:
  using LoadTokensResultCallback =
      base::Callback<void(std::vector<scoped_refptr<const AuthToken>>)>;

  ~WebdataClient() override {}

  virtual void ClearTokens() = 0;
  virtual void SaveTokens(
      std::vector<scoped_refptr<const AuthToken>> tokens) = 0;
  virtual void LoadTokens(LoadTokensResultCallback callback) = 0;
};
}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_TOKEN_CACHE_WEBDATA_CLIENT_H_
