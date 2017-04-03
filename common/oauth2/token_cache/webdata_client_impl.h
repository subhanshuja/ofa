// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_TOKEN_CACHE_WEBDATA_CLIENT_IMPL_H_
#define COMMON_OAUTH2_TOKEN_CACHE_WEBDATA_CLIENT_IMPL_H_

#include <string>

#include "base/bind.h"
#include "base/callback.h"

#include "common/oauth2/token_cache/token_cache_webdata.h"
#include "common/oauth2/token_cache/webdata_client.h"

namespace opera {
namespace oauth2 {

class WebdataClientImpl : public WebdataClient, public WebDataServiceConsumer {
 public:
  WebdataClientImpl();
  ~WebdataClientImpl() override;

  void Initialize(scoped_refptr<TokenCacheWebData> webdata);

  // KeyedService implementation.
  void Shutdown() override;

  void ClearTokens() override;
  void SaveTokens(std::vector<scoped_refptr<const AuthToken>> tokens) override;
  void LoadTokens(LoadTokensResultCallback callback) override;

  // WebDataServiceConsumer implementation.
  void OnWebDataServiceRequestDone(WebDataServiceBase::Handle h,
                                   const WDTypedResult* result) override;

 private:
  LoadTokensResultCallback on_tokens_loaded_callback_;
  WebDataServiceBase::Handle load_tokens_handle_;
  scoped_refptr<TokenCacheWebData> webdata_;
};
}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_TOKEN_CACHE_WEBDATA_CLIENT_IMPL_H_
