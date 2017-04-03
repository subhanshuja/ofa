// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SUGGESTION_SUGGESTION_ITEM_H_
#define COMMON_SUGGESTION_SUGGESTION_ITEM_H_

#include <string>

namespace opera {

struct SuggestionItem {
  SuggestionItem();
  SuggestionItem(
      int relevance, const std::string& title, const std::string& url,
      const std::string& type);

  int relevance;
  std::string title;
  std::string url;
  std::string type;

  bool operator<(const SuggestionItem& other) const {
    return relevance < other.relevance;
  }
};

}  // namespace opera

#endif  // COMMON_SUGGESTION_SUGGESTION_ITEM_H_
