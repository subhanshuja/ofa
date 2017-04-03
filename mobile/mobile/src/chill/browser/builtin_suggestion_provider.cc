// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/builtin_suggestion_provider.h"

#include <algorithm>

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/common/url_constants.h"
#include "ui/base/l10n/l10n_util.h"

#include "common/suggestion/suggestion_tokens.h"

#include "chill/common/opera_url_constants.h"

namespace opera {

namespace {

const char kShorthandSeparator = ':';

const std::string kOpera =
    std::string(content::kLegacyOperaUIScheme) + url::kStandardSchemeSeparator;
const std::string kShorthandOpera =
    std::string(content::kLegacyOperaUIScheme) + kShorthandSeparator;
const std::string kShorthandAbout =
    std::string(kOperaAboutScheme) + kShorthandSeparator;

const size_t kNoLimit = kNumberOfOperaHostUrls;

}  // namespace

BuiltinSuggestionProvider::BuiltinSuggestionProvider() {}

void BuiltinSuggestionProvider::Query(const SuggestionTokens& query,
    bool private_browsing,
    const std::string& type,
    const SuggestionCallback& callback) {
  const std::string& phrase = query.phrase();
  const size_t& terms = query.terms().size();

  std::vector<SuggestionItem> suggestions;

  if (base::StartsWith(phrase, kShorthandOpera,
                       base::CompareCase::INSENSITIVE_ASCII) ||
      base::StartsWith(phrase, kShorthandAbout,
                       base::CompareCase::INSENSITIVE_ASCII)) {
    std::string token;
    int limit = kNoLimit;

    // SuggestionTokens doesn't tokenize on ':' or '/', but does on "://",
    // meaning we'll have to fiddle around to figure out what's what itself
    // SuggestionTokens itself uses base::i18n::BreakIterator from
    // Chromium to split strings into tokens. If the behaviour of BreakIterator
    // would change and tokenization would be performed on non-word characters
    // (such as ':'), the following will break.
    if (terms == 2) {
      token = query.terms().back();
    } else {
      size_t separator = phrase.find(':');
      token = phrase.substr(separator+1);
      if (token == "//") {
        // User typed 'opera://'. Enforce no limit on suggestions and match on
        // all builtins.
        token = std::string();
      } else {
        limit = kMaxSuggestions;
      }
    }

    AddSuggestionsFrom(token, limit, kRelevance, type, &suggestions);
  } else {
    // Allow for one low-priority suggestion when the user has typed at least
    // three matching characters, with or without 'opera' (or 'about') as the
    // first token. Example: 'opera de' will not suggest anything, while
    // 'opera deb' will suggest 'opera://debug'.
    const std::string& head = query.terms().front();
    std::string term;
    if (terms == 2 &&
        (head == content::kLegacyOperaUIScheme || head == kOperaAboutScheme) &&
        query.terms().back().size() >= 3)
      term = query.terms().back();
    else if (terms == 1 && head.size() >= 3)
      term = head;

    if (!term.empty())
      AddSuggestionsFrom(term, 1, kLowRelevance, type, &suggestions);
  }

  callback.Run(suggestions);
}

void BuiltinSuggestionProvider::AddSuggestionsFrom(
    const std::string& term,
    size_t limit,
    int relevance,
    const std::string& type,
    SuggestionList* list) {
  for (size_t i = 0; i < kNumberOfOperaHostUrls && i < limit; i++) {
    std::string host(kOperaHostUrls[i].url);
    if (base::StartsWith(host, term,
                         base::CompareCase::INSENSITIVE_ASCII)) {
      base::string16 title = l10n_util::GetStringUTF16(kOperaHostUrls[i].title);
      list->push_back(SuggestionItem(relevance, base::UTF16ToUTF8(title),
                                     kOpera + host, type));
    }
  }
}

}  // namespace opera
