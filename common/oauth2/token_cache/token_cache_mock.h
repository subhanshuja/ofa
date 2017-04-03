// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_TOKEN_CACHE_TOKEN_CACHE_MOCK_H_
#define COMMON_OAUTH2_TOKEN_CACHE_TOKEN_CACHE_MOCK_H_

#include "testing/gmock/include/gmock/gmock.h"

#include "common/oauth2/token_cache/token_cache.h"

namespace opera {
namespace oauth2 {
class TokenCacheMock : public TokenCache {
 public:
  TokenCacheMock();
  ~TokenCacheMock() override;

  MOCK_METHOD1(Load, void(base::Closure));
  MOCK_METHOD0(Store, void());
  MOCK_METHOD0(Clear, void());
  MOCK_METHOD2(GetToken,
               scoped_refptr<const AuthToken>(const std::string&,
                                              const ScopeSet&));
  MOCK_METHOD1(PutToken, void(scoped_refptr<const AuthToken>));
  MOCK_METHOD1(EvictToken, void(scoped_refptr<const AuthToken>));
};
}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_TOKEN_CACHE_TOKEN_CACHE_MOCK_H_
