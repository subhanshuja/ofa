// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/util/token.h"

#include "components/sync/api/time.h"

namespace opera {
namespace oauth2 {
AuthToken::AuthToken(const std::string& client_name,
                     const std::string& token,
                     const ScopeSet& scopes,
                     const base::Time& expiration_time)
    : client_name_(client_name),
      token_(token),
      scopes_(scopes),
      expiration_time_(expiration_time) {}

AuthToken::~AuthToken() {}

std::string AuthToken::client_name() const {
  return client_name_;
}

std::string AuthToken::token() const {
  return token_;
}

ScopeSet AuthToken::scopes() const {
  return scopes_;
}

base::Time AuthToken::expiration_time() const {
  return expiration_time_;
}

bool AuthToken::is_expired() const {
  return expiration_time() < base::Time::Now();
}

bool AuthToken::is_valid() const {
  if (client_name_.empty())
    return false;

  if (token_.empty())
    return false;

  if (scopes_.empty())
    return false;

  if (expiration_time_ == base::Time())
    return false;

  return true;
}

bool AuthToken::operator==(const AuthToken& other) const {
  if (client_name() != other.client_name())
    return false;

  if (token() != other.token())
    return false;

  if (scopes() != other.scopes())
    return false;

  if (expiration_time() != other.expiration_time())
    return false;

  return true;
}

const std::string AuthToken::GetDiagnosticDescription() const {
  return "client_name='" + client_name() + "', scopes='" + scopes().encode() +
         "', expiration_time='" +
         syncer::GetTimeDebugString(expiration_time());
}

}  // namespace oauth2
}  // namespace opera
