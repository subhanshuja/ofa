// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_TOKEN_CACHE_TOKEN_CACHE_H_
#define COMMON_OAUTH2_TOKEN_CACHE_TOKEN_CACHE_H_

#include <string>

#include "base/callback.h"
#include "components/keyed_service/core/keyed_service.h"

#include "common/oauth2/util/token.h"

namespace opera {
namespace oauth2 {

class TokenCache: public KeyedService {
 public:
  ~TokenCache() override {}

  virtual void Load(base::Closure load_complete_callback) = 0;
  virtual void Store() = 0;
  virtual void Clear() = 0;

  virtual scoped_refptr<const AuthToken> GetToken(
      const std::string& client_name, const ScopeSet& scopes) = 0;
  virtual void PutToken(scoped_refptr<const AuthToken> token) = 0;
  virtual void EvictToken(scoped_refptr<const AuthToken> token) = 0;
};
}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_TOKEN_CACHE_TOKEN_CACHE_H_
