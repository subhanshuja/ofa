// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_UTIL_SCOPE_SET_H_
#define COMMON_OAUTH2_UTIL_SCOPE_SET_H_

#include <string>
#include <set>

namespace opera {
namespace oauth2 {

class ScopeSet {
 public:
  ScopeSet();
  ScopeSet(const std::set<std::string> scopes);
  ScopeSet(const ScopeSet&);

  static ScopeSet FromEncoded(const std::string& encoded);

  ~ScopeSet();

  bool operator==(const ScopeSet& other) const;
  bool operator!=(const ScopeSet& other) const;

  std::string encode() const;
  void decode(const std::string& scope_set);
  size_t size() const;
  bool empty() const;
  bool has_scope(const std::string& scope) const;

 private:
  std::set<std::string> scopes_;
};
}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_UTIL_SCOPE_SET_H_
