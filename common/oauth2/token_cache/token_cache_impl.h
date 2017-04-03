// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_OAUTH2_TOKEN_CACHE_TOKEN_CACHE_IMPL_H_
#define COMMON_OAUTH2_OAUTH2_TOKEN_CACHE_TOKEN_CACHE_IMPL_H_

#include <string>
#include <map>

#include "base/callback.h"

#include "common/oauth2/diagnostics/diagnostic_service.h"
#include "common/oauth2/diagnostics/diagnostic_supplier.h"
#include "common/oauth2/util/token.h"
#include "common/oauth2/token_cache/token_cache_webdata.h"

#include "common/oauth2/token_cache/token_cache.h"
#include "common/oauth2/token_cache/webdata_client.h"

namespace opera {
namespace oauth2 {

class TokenCacheImpl : public TokenCache, public DiagnosticSupplier {
 public:
  TokenCacheImpl(DiagnosticService* diagnostic_service);
  ~TokenCacheImpl() override;

  void Initialize(WebdataClient* webdata_client);

  void Shutdown() override;

  // DiagnosticSupplier implementation.
  std::string GetDiagnosticName() const override;
  std::unique_ptr<base::DictionaryValue> GetDiagnosticSnapshot() override;

  void Load(base::Closure load_complete_callback) override;
  void Store() override;
  void Clear() override;

  scoped_refptr<const AuthToken> GetToken(const std::string& client_name,
                                          const ScopeSet& scopes) override;
  void PutToken(scoped_refptr<const AuthToken> token) override;
  void EvictToken(scoped_refptr<const AuthToken> token) override;

  void OnTokensLoaded(std::vector<scoped_refptr<const AuthToken>> tokens);

 private:
  bool HasToken(const std::string& key) const;

  DiagnosticService* diagnostic_service_;

  std::map<std::string, scoped_refptr<const AuthToken>> cache_;
  base::Closure cache_loaded_callback_;

  WebdataClient* webdata_client_;

  base::WeakPtrFactory<TokenCacheImpl> weak_ptr_factory_;
};
}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_OAUTH2_TOKEN_CACHE_TOKEN_CACHE_IMPL_H_
