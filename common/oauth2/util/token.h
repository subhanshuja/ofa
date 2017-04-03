// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_UTIL_TOKEN_H_
#define COMMON_OAUTH2_UTIL_TOKEN_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "base/time/time.h"

#include "common/oauth2/util/scope_set.h"

namespace opera {
namespace oauth2 {
class AuthToken : public base::RefCounted<AuthToken> {
 public:
  AuthToken(const std::string& client_name,
            const std::string& token,
            const ScopeSet& scopes,
            const base::Time& expiration_time);

  std::string client_name() const;
  std::string token() const;
  ScopeSet scopes() const;
  base::Time expiration_time() const;

  bool is_expired() const;
  bool is_valid() const;

  bool operator==(const AuthToken& other) const;

  const std::string GetDiagnosticDescription() const;

 private:
  friend class base::RefCounted<AuthToken>;
  ~AuthToken();

  std::string client_name_;
  std::string token_;
  ScopeSet scopes_;
  base::Time expiration_time_;
};
}  // namespace oauth2
}  // namespace opera

#endif  // COMMON_OAUTH2_UTIL_TOKEN_H_
