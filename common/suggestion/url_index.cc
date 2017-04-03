// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/suggestion/url_index.h"

#include <algorithm>

#include "base/i18n/case_conversion.h"
#include "base/strings/utf_string_conversions.h"
#include "net/base/escape.h"
#include "url/gurl.h"

#include "common/suggestion/query_parser.h"

namespace opera {

namespace {

enum { FIRST_VALID_TERM_ID = 1 };

static const size_t kCleanedUpUrlMaxLength = 1024u;
static const size_t kCleanedUpTitleMaxLength = 1024u;

// Only this many characters of any term from indexed entry is remembered.
static const size_t kMaxSignificantChars = 50u;

bool HasPrefix(const base::string16& text, const base::string16& prefix) {
  if (text.size() < prefix.size())
    return false;
  return text.compare(0, prefix.size(), prefix) == 0;
}

// Our custom URL formatting for the index. Note that we cannot use
// net/net_util.h here, because it references part of ICU library that are
// not available on iOS, and we consider providing our own version of that
// whole library prohibitively costly.

base::string16 StripFormattedUrl(const base::string16& url) {
  static const base::string16 urlPrefixes[] = {
    base::UTF8ToUTF16("www."),
    base::UTF8ToUTF16("ftp."),
    base::string16(),
  };

  for (size_t i = 0; !urlPrefixes[i].empty(); ++i) {
    if (HasPrefix(url, urlPrefixes[i])) {
      return url.substr(urlPrefixes[i].size());
    }
  }
  return url;
}

base::string16 FormatUrl(GURL url) {
  if (!url.is_valid())
    return base::string16();

  if (url.has_username() || url.has_password()) {
    url::Replacements<char> replacements;
    replacements.ClearUsername();
    replacements.ClearPassword();
    url = url.ReplaceComponents(replacements);
  }

  return net::UnescapeAndDecodeUTF8URLComponent(
      url.GetContent(), net::UnescapeRule::SPACES |
      net::UnescapeRule::REPLACE_PLUS_WITH_SPACE);
}

base::string16 CleanUpUrl(const GURL& url) {
  base::string16 formatted_url = base::i18n::ToLower(
      FormatUrl(url).substr(0, kCleanedUpUrlMaxLength));
  return StripFormattedUrl(formatted_url);
}

base::string16 CleanUpTitle(const base::string16& title) {
  return base::i18n::ToLower(title.substr(0, kCleanedUpTitleMaxLength));
}

}  // namespace

URLIndex::URLIndex() : next_id_(FIRST_VALID_TERM_ID) {}

URLIndex::~URLIndex() {}

void URLIndex::IndexEntry(
        URLIndex::ID id, const base::string16& title, const GURL& url) {
  TermIDSet term_ids;
  AddTermIDsForEntry(id, title, url, &term_ids);
  RegisterID(id, term_ids);
}

void URLIndex::RemoveEntry(URLIndex::ID id) {
  UnregisterID(id);
}

void URLIndex::Clear() {
  index_.clear();
  rev_index_.clear();
  term_ids_.clear();
  terms_.clear();
  next_id_ = FIRST_VALID_TERM_ID;
}

bool URLIndex::IsEmpty() const {
  return index_.empty();
}

URLIndex::URLIDSet URLIndex::Search(
    const std::vector<query_parser::QueryNode*>& query_nodes) const {
  if (query_nodes.empty())
    return URLIDSet();
  // Extract terms from query nodes.
  std::vector<Term> term_vector;
  size_t n = query_nodes.size();
  for (size_t i = 0; i < n; ++i) {
    query_nodes[i]->AppendWords(&term_vector);
  }

  // Deduplicate words, and truncate them to significant character limit.
  base::hash_set<Term> query_terms;
  for (size_t i = 0, n = term_vector.size(); i < n; ++i) {
    query_terms.insert(term_vector[i].substr(0, kMaxSignificantChars));
  }

  // Intersect id sets for all truncated terms.
  base::hash_set<Term>::const_iterator i = query_terms.begin();
  URLIDSet matching_ids = SearchForTerm(*i++);
  for (; i != query_terms.end() && matching_ids.size(); ++i) {
    URLIDSet ids_for_term = SearchForTerm(*i);
    URLIDSet new_set;
    std::set_intersection(matching_ids.begin(), matching_ids.end(),
                          ids_for_term.begin(), ids_for_term.end(),
                          std::inserter(new_set, new_set.begin()));
    matching_ids.swap(new_set);
  }
  return matching_ids;
}

URLIndex::TermID URLIndex::GetTermID(const Term& term) {
  TermMap::const_iterator i = term_ids_.find(term);
  if (i != term_ids_.end())
    return i->second;
  term_ids_[term] = next_id_;
  terms_[next_id_] = term;
  return next_id_++;
}

void URLIndex::RemoveTermID(TermID id) {
  const Term& term = terms_[id];
  index_.erase(term);
  term_ids_.erase(term);
  terms_.erase(id);

  if (terms_.empty())
    next_id_ = FIRST_VALID_TERM_ID;
}

void URLIndex::AddTermIDsForText(const base::string16& text,
                                 TermIDSet* ids) {
  DCHECK(ids);
  std::vector<query_parser::QueryWord> words;
  query_parser::QueryParser query_parser;
  query_parser.ExtractQueryWords(text, &words);
  if (words.empty())
    return;

  size_t n = words.size();
  for (size_t i = 0; i < n; ++i) {
    ids->insert(GetTermID(words[i].word.substr(0, kMaxSignificantChars)));
  }
}

void URLIndex::AddTermIDsForEntry(URLIndex::ID id,
                                  const base::string16& title,
                                  const GURL& url,
                                  TermIDSet* term_ids) {
  DCHECK(term_ids);
  base::string16 text = CleanUpTitle(title);
  if (!text.empty()) {
    AddTermIDsForText(text, term_ids);
  }

  text = CleanUpUrl(url);
  if (!text.empty()) {
    AddTermIDsForText(text, term_ids);
  }
}

void URLIndex::RegisterID(URLIndex::ID id,
                          const TermIDSet& term_ids) {
  for (TermIDSet::const_iterator i = term_ids.begin();
       i != term_ids.end();
       ++i) {
    index_[terms_[*i]].insert(id);
  }
  rev_index_[id] = term_ids;
}

void URLIndex::UnregisterID(URLIndex::ID id) {
  const TermIDSet& term_ids = rev_index_[id];
  for (TermIDSet::const_iterator i = term_ids.begin();
       i != term_ids.end();
       ++i) {
    URLIDSet& entry_set = index_[terms_[*i]];
    entry_set.erase(id);
    if (entry_set.empty())
      RemoveTermID(*i);
  }
  rev_index_.erase(id);
}

URLIndex::URLIDSet URLIndex::SearchForTerm(const Term& word) const {
  URLIDSet matching_ids;
  TermURLIDMap::const_iterator i = index_.lower_bound(word);
  if (i == index_.end())
    return matching_ids;  // No interesting matches.
  if (!query_parser::QueryParser::IsWordLongEnoughForPrefixSearch(
          word, query_parser::MatchingAlgorithm::DEFAULT)) {
    if (i->first.compare(word) != 0)
      return matching_ids;  // No exact matches.
    return i->second;
  }

  // Sum id sets for all keys starting with word.
  for (; i != index_.end() && HasPrefix(i->first, word); ++i) {
    URLIDSet new_set;
    const URLIDSet& next_set = i->second;
    std::set_union(matching_ids.begin(), matching_ids.end(),
                   next_set.begin(), next_set.end(),
                   std::inserter(new_set, new_set.begin()));
    matching_ids.swap(new_set);
  }
  return matching_ids;
}

}  // namespace opera
