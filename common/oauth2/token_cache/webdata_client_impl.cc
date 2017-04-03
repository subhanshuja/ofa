// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/token_cache/webdata_client_impl.h"

namespace opera {
namespace oauth2 {

WebdataClientImpl::WebdataClientImpl() : load_tokens_handle_(-1) {}

WebdataClientImpl::~WebdataClientImpl() {}

void WebdataClientImpl::Initialize(scoped_refptr<TokenCacheWebData> webdata) {
  DCHECK(webdata);
  webdata_ = webdata;
}

void WebdataClientImpl::Shutdown() {}

void WebdataClientImpl::ClearTokens() {
  webdata_->ClearTokens();
}

void WebdataClientImpl::SaveTokens(
    std::vector<scoped_refptr<const AuthToken>> tokens) {
  webdata_->SaveTokens(tokens);
}

void WebdataClientImpl::LoadTokens(LoadTokensResultCallback callback) {
  DCHECK_EQ(-1, load_tokens_handle_);
  DCHECK(!callback.is_null());
  DCHECK(on_tokens_loaded_callback_.is_null());
  on_tokens_loaded_callback_ = callback;
  load_tokens_handle_ = webdata_->LoadTokens(this);
}

void WebdataClientImpl::OnWebDataServiceRequestDone(
    WebDataServiceBase::Handle h,
    const WDTypedResult* result) {
  DCHECK(!on_tokens_loaded_callback_.is_null());
  DCHECK_EQ(load_tokens_handle_, h);
  load_tokens_handle_ = -1;
  DCHECK(result);
  // See http://crbug.com/68783.
  if (!result) {
    return;
  }

  DCHECK_EQ(TOKEN_RESULT, result->GetType());
  std::vector<scoped_refptr<const AuthToken>> tokens =
      static_cast<const WDResult<std::vector<scoped_refptr<const AuthToken>>>*>(
          result)
          ->GetValue();

  on_tokens_loaded_callback_.Run(tokens);
  on_tokens_loaded_callback_.Reset();
}

}  // namespace oauth2
}  // namespace opera
