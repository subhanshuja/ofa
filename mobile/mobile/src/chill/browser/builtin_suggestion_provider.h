// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_BUILTIN_SUGGESTION_PROVIDER_H_
#define CHILL_BROWSER_BUILTIN_SUGGESTION_PROVIDER_H_

#include <set>
#include <string>
#include <vector>

#include "common/suggestion/suggestion_callback.h"
#include "common/suggestion/suggestion_item.h"
#include "common/suggestion/suggestion_provider.h"

namespace opera {

class SuggestionTokens;

class BuiltinSuggestionProvider : public SuggestionProvider {
 public:
  BuiltinSuggestionProvider();

  void Query(const SuggestionTokens& query,
             bool private_browsing,
             const std::string& type,
             const SuggestionCallback& callback);

  void Cancel() { }

 private:
  typedef std::vector<SuggestionItem> SuggestionList;

  void AddSuggestionsFrom(const std::string& term,
                          size_t limit,
                          int relevance,
                          const std::string& type,
                          SuggestionList* list);

  static const int kMaxSuggestions = 3;
  static const int kRelevance = 720;  // Somewhat higher than search suggestions
  static const int kLowRelevance = 200;

  DISALLOW_COPY_AND_ASSIGN(BuiltinSuggestionProvider);
};

}  // namespace opera

#endif  // CHILL_BROWSER_BUILTIN_SUGGESTION_PROVIDER_H_
