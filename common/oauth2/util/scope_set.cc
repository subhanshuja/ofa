// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include <algorithm>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

#include "common/oauth2/util/scope_set.h"

namespace opera {
namespace oauth2 {

ScopeSet::ScopeSet() {}
ScopeSet::ScopeSet(const std::set<std::string> scopes)
    : scopes_(scopes) {}

ScopeSet::ScopeSet(const ScopeSet&) = default;

// static
ScopeSet ScopeSet::FromEncoded(const std::string& encoded) {
  ScopeSet scopes;
  scopes.decode(encoded);
  return scopes;
}

ScopeSet::~ScopeSet() {}

bool ScopeSet::operator==(const ScopeSet& other) const {
  return size() == other.size() &&
         std::equal(scopes_.begin(), scopes_.end(), other.scopes_.begin());
}

bool ScopeSet::operator!=(const ScopeSet& other) const {
  return !(*this == other);
}

std::string ScopeSet::encode() const {
  std::string encoded;
  for (auto scope : scopes_) {
    encoded += scope + " ";
  }
  return base::TrimString(encoded, " ", base::TrimPositions::TRIM_TRAILING)
      .as_string();
}

void ScopeSet::decode(const std::string& scope_set) {
  scopes_.clear();
  std::vector<std::string> parsed = SplitString(
      scope_set, " ", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  for (const auto& p : parsed) {
    scopes_.insert(p);
  }
}

size_t ScopeSet::size() const {
  return scopes_.size();
}

bool ScopeSet::empty() const {
  return scopes_.empty();
}

bool ScopeSet::has_scope(const std::string& scope) const {
  return scopes_.find(scope) != scopes_.end();
}

}  // namespace oauth2
}  // namespace opera
