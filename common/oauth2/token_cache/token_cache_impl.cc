// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/token_cache/token_cache_impl.h"

#include "base/bind.h"

#include "common/oauth2/token_cache/token_cache_table.h"
#include "common/oauth2/util/token.h"
#include "common/oauth2/util/util.h"

namespace opera {
namespace oauth2 {

namespace {
std::string AsRequestKey(const std::string& client_name,
                         const ScopeSet& scopes) {
  return client_name + scopes.encode();
}
}

TokenCacheImpl::TokenCacheImpl(DiagnosticService* diagnostic_service)
    : diagnostic_service_(diagnostic_service), weak_ptr_factory_(this) {
  VLOG(1) << "Token cache created.";
  if (diagnostic_service_) {
    diagnostic_service_->AddSupplier(this);
  }
}

TokenCacheImpl::~TokenCacheImpl() {
  VLOG(1) << "Token cache destroyed.";
  if (diagnostic_service_) {
    diagnostic_service_->RemoveSupplier(this);
  }
}

void TokenCacheImpl::Initialize(WebdataClient* webdata_client) {
  VLOG(1) << "Token cache initializing.";
  DCHECK(webdata_client);
  webdata_client_ = webdata_client;
}

void TokenCacheImpl::Shutdown() {}

std::string TokenCacheImpl::GetDiagnosticName() const {
  return "token_cache";
}

std::unique_ptr<base::DictionaryValue> TokenCacheImpl::GetDiagnosticSnapshot() {
  std::unique_ptr<base::DictionaryValue> snapshot(new base::DictionaryValue);
  std::unique_ptr<base::ListValue> tokens(new base::ListValue);
  for (const auto& item : cache_) {
    std::unique_ptr<base::DictionaryValue> token_info(
        new base::DictionaryValue);
    token_info->SetString("client_name", item.second->client_name());
    token_info->SetString("scopes", item.second->scopes().encode());
    token_info->SetDouble("expiration_time",
                          item.second->expiration_time().ToJsTime());
    tokens->Append(std::move(token_info));
  }
  snapshot->Set("tokens", std::move(tokens));
  return snapshot;
}

void TokenCacheImpl::Load(base::Closure load_complete_callback) {
  DCHECK(!load_complete_callback.is_null());

  VLOG(1) << "Token cache loading.";
  cache_loaded_callback_ = load_complete_callback;
  webdata_client_->LoadTokens(base::Bind(&TokenCacheImpl::OnTokensLoaded,
                                         weak_ptr_factory_.GetWeakPtr()));
}

void TokenCacheImpl::Store() {
  VLOG(1) << "Token cache storing state.";
  webdata_client_->ClearTokens();

  std::vector<scoped_refptr<const AuthToken>> tokens;

  for (const auto& cached_entry : cache_) {
    const auto& token = cached_entry.second;
    VLOG(3) << "Cache has an access token for client '" << token->client_name()
            << "' and scopes '" << token->scopes().encode() << "', expires at "
            << token->expiration_time();
    if (!token->is_expired()) {
      tokens.push_back(token.get());
      VLOG(1) << "Token becomes stored.";
    } else {
      VLOG(1) << "Token is expired, dropping.";
    }
  }

  VLOG(1) << "Token cache storing " << tokens.size() << " tokens.";
  webdata_client_->SaveTokens(tokens);
}

void TokenCacheImpl::Clear() {
  VLOG(1) << "Token cache clearing.";
  cache_.clear();
  webdata_client_->ClearTokens();

  if (diagnostic_service_) {
    diagnostic_service_->TakeSnapshot();
  }
}

scoped_refptr<const AuthToken> TokenCacheImpl::GetToken(
    const std::string& client_name,
    const ScopeSet& scopes) {
  DCHECK(!client_name.empty());
  DCHECK(!scopes.empty());

  const auto request_key = AsRequestKey(client_name, scopes);
  VLOG(1) << "Token cache asked for '" << request_key << "'.";

  if (!HasToken(request_key)) {
    VLOG(1) << "Not known.";
    return nullptr;
  }

  scoped_refptr<const AuthToken> token = cache_.at(request_key).get();
  if (token->is_expired()) {
    VLOG(1) << "Expired.";
    EvictToken(token);
    return nullptr;
  }

  VLOG(1) << "Found.";
  return token;
}

void TokenCacheImpl::PutToken(scoped_refptr<const AuthToken> token) {
  DCHECK(token.get());

  if (!token->is_valid()) {
    VLOG(1) << "Cache got invalid token '" << token->GetDiagnosticDescription()
            << "', ignoring.";
    return;
  }

  if (token->is_expired()) {
    VLOG(1) << "Cache got expired token '" << token->GetDiagnosticDescription()
            << "', ignoring.";
    return;
  }

  const auto request_key = AsRequestKey(token->client_name(), token->scopes());
  DCHECK(!HasToken(request_key));

  VLOG(1) << "Caching token " << token->client_name() << " "
          << token->scopes().encode() << " " << token->expiration_time() << " ("
          << request_key << ")";
  cache_[request_key] = token;

  if (diagnostic_service_) {
    diagnostic_service_->TakeSnapshot();
  }
}

void TokenCacheImpl::EvictToken(scoped_refptr<const AuthToken> token) {
  DCHECK(token);

  const auto request_key = AsRequestKey(token->client_name(), token->scopes());
  VLOG(1) << "Cache dropping token " << token->GetDiagnosticDescription()
          << " (" << request_key << ")";

  // Lookup the token in cache.
  std::string cache_key;
  for (const auto& cache_entry : cache_) {
    const auto& token_it = cache_entry.second;
    DCHECK(token_it);
    if (token_it == token) {
      cache_key = cache_entry.first;
      break;
    }
  }

  DCHECK(!cache_key.empty());
  cache_.erase(cache_key);

  if (diagnostic_service_) {
    diagnostic_service_->TakeSnapshot();
  }
}

bool TokenCacheImpl::HasToken(const std::string& key) const {
  return (cache_.count(key) != 0);
}

void TokenCacheImpl::OnTokensLoaded(
    std::vector<scoped_refptr<const AuthToken>> tokens) {
  DCHECK(cache_.empty());

  for (auto token : tokens) {
    VLOG(3) << "Found an access token in storage: client '"
            << token->client_name() << "' and scopes '"
            << token->scopes().encode() << "', expires at "
            << token->expiration_time();
    if (token->is_expired()) {
      VLOG(3) << "Token is expired, dropping on the floor.";
      continue;
    }
    if (!token->is_valid()) {
      VLOG(3) << "Token is invalid, dropping on the floor.";
      continue;
    }
    PutToken(token);
  }

  VLOG(1) << "Restored " << cache_.size() << " access tokens.";
  cache_loaded_callback_.Run();
}

}  // namespace oauth2
}  // namespace opera
