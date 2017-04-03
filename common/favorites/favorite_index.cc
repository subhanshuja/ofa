// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/browser/bookmarks/bookmark_index.cc
// @final-synchronized bdec20a2da9cf437cd0e52ee57c9572eddcfa02e

#include "common/favorites/favorite_index.h"

#include <algorithm>
#include <iterator>
#include <list>

#include "base/i18n/case_conversion.h"
#include "base/memory/scoped_vector.h"
#include "base/strings/string16.h"
#include "ui/base/l10n/l10n_util.h"

#include "common/suggestion/query_parser.h"

#include "common/favorites/favorite.h"
#include "common/favorites/favorite_utils.h"

namespace opera {

// Used when finding the set of bookmarks that match a query. Each match
// represents a set of terms (as an interator into the Index) matching the
// query as well as the set of nodes that contain those terms in their titles.
struct FavoriteIndex::Match {
  // List of terms matching the query.
  std::list<Index::const_iterator> terms;

  // The set of nodes matching the terms. As an optimization this is empty
  // when we match only one term, and is filled in when we get more than one
  // term. We can do this as when we have only one matching term we know
  // the set of matching nodes is terms.front()->second.
  //
  // Use nodes_begin() and nodes_end() to get an iterator over the set as
  // it handles the necessary switching between nodes and terms.front().
  NodeSet nodes;

  // Returns an iterator to the beginning of the matching nodes. See
  // description of nodes for why this should be used over nodes.begin().
  NodeSet::const_iterator nodes_begin() const;

  // Returns an iterator to the beginning of the matching nodes. See
  // description of nodes for why this should be used over nodes.end().
  NodeSet::const_iterator nodes_end() const;
};

FavoriteIndex::NodeSet::const_iterator
    FavoriteIndex::Match::nodes_begin() const {
  return nodes.empty() ? terms.front()->second.begin() : nodes.begin();
}

FavoriteIndex::NodeSet::const_iterator FavoriteIndex::Match::nodes_end() const {
  return nodes.empty() ? terms.front()->second.end() : nodes.end();
}

FavoriteIndex::FavoriteIndex() {
}

FavoriteIndex::~FavoriteIndex() {
}

void FavoriteIndex::Add(const Favorite* fav) {
  if (fav->display_url().is_empty() || !fav->AllowURLIndexing())
    return;

  const FavoriteKeywords& terms = fav->GetKeywords();
  for (size_t i = 0; i < terms.size(); ++i)
    RegisterNode(terms[i], fav);
}

void FavoriteIndex::Remove(const Favorite* fav) {
  if (fav->display_url().is_empty())
    return;

  const FavoriteKeywords& terms = fav->GetKeywords();
  for (size_t i = 0; i < terms.size(); ++i)
    UnregisterNode(fav);
}

void FavoriteIndex::GetFavoritesWithTitlesMatching(
    const base::string16& query,
    size_t max_count,
    std::vector<FavoriteUtils::TitleMatch>* results) {
  std::vector<base::string16> terms = ExtractQueryWords(query);
  if (terms.empty())
    return;

  Matches matches;
  for (size_t i = 0; i < terms.size(); ++i) {
    if (!GetFavoritesWithTitleMatchingTerm(terms[i], i == 0, &matches))
      return;
  }

  NodeTypedCountPairs node_typed_counts;
  SortMatches(matches, &node_typed_counts);

  // We use a QueryParser to fill in match positions for us. It's not the most
  // efficient way to go about this, but by the time we get here we know what
  // matches and so this shouldn't be performance critical.
  query_parser::QueryParser parser;
  ScopedVector<query_parser::QueryNode> query_nodes;
  parser.ParseQueryNodes(query,
                         query_parser::MatchingAlgorithm::DEFAULT,
                         &query_nodes.get());

  // The highest typed counts should be at the beginning of the results vector
  // so that the best matches will always be included in the results. The loop
  // that calculates result relevance in HistoryContentsProvider::ConvertResults
  // will run backwards to assure higher relevance will be attributed to the
  // best matches.
  for (NodeTypedCountPairs::const_iterator i = node_typed_counts.begin();
       i != node_typed_counts.end() && results->size() < max_count; ++i)
    AddMatchToResults(i->first, &parser, query_nodes.get(), results);
}


void FavoriteIndex::SortMatches(const Matches& matches,
                                NodeTypedCountPairs* node_typed_counts) const {
  // TODO(kubal): Missing typed count from history!
  for (Matches::const_iterator i = matches.begin(); i != matches.end(); ++i)
    ExtractFavoriteNodePairs(*i, node_typed_counts);

  // Eliminate duplicates.
  node_typed_counts->erase(std::unique(node_typed_counts->begin(),
                                       node_typed_counts->end()),
                           node_typed_counts->end());
}

void FavoriteIndex::ExtractFavoriteNodePairs(const Match& match,
    NodeTypedCountPairs* node_typed_counts) const {
  // TODO(kubal): Missing typed count from history!
  for (NodeSet::const_iterator i = match.nodes_begin();
       i != match.nodes_end(); ++i) {
    NodeTypedCountPair pair(*i, 0);  // TODO(kubal): fix count!
    node_typed_counts->push_back(pair);
  }
}

void FavoriteIndex::AddMatchToResults(
    const Favorite* fav,
    query_parser::QueryParser* parser,
    const std::vector<query_parser::QueryNode*>& query_nodes,
    std::vector<FavoriteUtils::TitleMatch>* results) {
  FavoriteUtils::TitleMatch title_match;
  // Check that the result matches the query.  The previous search
  // was a simple per-word search, while the more complex matching
  // of QueryParser may filter it out.  For example, the query
  // ["thi"] will match the bookmark titled [Thinking], but since
  // ["thi"] is quoted we don't want to do a prefix match.
  if (parser->DoesQueryMatch(fav->title(), query_nodes,
                             &(title_match.match_positions))) {
    title_match.node = fav;
    results->push_back(title_match);
  }
}

bool FavoriteIndex::GetFavoritesWithTitleMatchingTerm(const base::string16& term,
                                                      bool first_term,
                                                      Matches* matches) {
  Index::const_iterator i = index_.lower_bound(term);
  if (i == index_.end())
    return false;

  if (!query_parser::QueryParser::IsWordLongEnoughForPrefixSearch(
          term, query_parser::MatchingAlgorithm::DEFAULT)) {
    // Term is too short for prefix match, compare using exact match.
    if (i->first != term)
      return false;  // No bookmarks with this term.

    if (first_term) {
      Match match;
      match.terms.push_back(i);
      matches->push_back(match);
      return true;
    }
    CombineMatchesInPlace(i, matches);
  } else if (first_term) {
    // This is the first term and we're doing a prefix match. Loop through
    // index adding all entries that start with term to matches.
    while (i != index_.end() &&
           i->first.size() >= term.size() &&
           term.compare(0, term.size(), i->first, 0, term.size()) == 0) {
      Match match;
      match.terms.push_back(i);
      matches->push_back(match);
      ++i;
    }
  } else {
    // Prefix match and not the first term. Loop through index combining
    // current matches in matches with term, placing result in result.
    Matches result;
    while (i != index_.end() &&
           i->first.size() >= term.size() &&
           term.compare(0, term.size(), i->first, 0, term.size()) == 0) {
      CombineMatches(i, *matches, &result);
      ++i;
    }
    matches->swap(result);
  }
  return !matches->empty();
}

void FavoriteIndex::CombineMatchesInPlace(const Index::const_iterator& index_i,
                                          Matches* matches) {
  for (size_t i = 0; i < matches->size(); ) {
    Match* match = &((*matches)[i]);
    NodeSet intersection;
    std::set_intersection(match->nodes_begin(), match->nodes_end(),
                          index_i->second.begin(), index_i->second.end(),
                          std::inserter(intersection, intersection.begin()));
    if (intersection.empty()) {
      matches->erase(matches->begin() + i);
    } else {
      match->terms.push_back(index_i);
      match->nodes.swap(intersection);
      ++i;
    }
  }
}

void FavoriteIndex::CombineMatches(const Index::const_iterator& index_i,
                                   const Matches& current_matches,
                                   Matches* result) {
  for (size_t i = 0; i < current_matches.size(); ++i) {
    const Match& match = current_matches[i];
    NodeSet intersection;
    std::set_intersection(match.nodes_begin(), match.nodes_end(),
                          index_i->second.begin(), index_i->second.end(),
                          std::inserter(intersection, intersection.begin()));
    if (!intersection.empty()) {
      result->push_back(Match());
      Match& combined_match = result->back();
      combined_match.terms = match.terms;
      combined_match.terms.push_back(index_i);
      combined_match.nodes.swap(intersection);
    }
  }
}

std::vector<base::string16> FavoriteIndex::ExtractQueryWords(const base::string16& query) {
  std::vector<base::string16> terms;
  if (query.empty())
    return std::vector<base::string16>();
  query_parser::QueryParser parser;
  // TODO(brettw): use ICU normalization:
  // http://userguide.icu-project.org/transforms/normalization
  parser.ParseQueryWords(base::i18n::ToLower(query),
                         query_parser::MatchingAlgorithm::DEFAULT,
                         &terms);
  return terms;
}

void FavoriteIndex::RegisterNode(const base::string16& term,
                                 const Favorite* node) {
  if (std::find(index_[term].begin(), index_[term].end(), node) !=
      index_[term].end()) {
    // We've already added node for term.
    return;
  }
  index_[term].insert(node);
  favs_idx_[node].insert(term);
}

void FavoriteIndex::UnregisterNode(const Favorite* node) {
  FavsIdx::iterator i = favs_idx_.find(node);
  if (i == favs_idx_.end())
    return;

  for (KeywordsSet::iterator k = i->second.begin(); k != i->second.end(); ++k) {
    Index::iterator keyword = index_.find(*k);
    if (keyword == index_.end()) {
      // We can get here if the node has the same term more than once. For
      // example, a bookmark with the title 'foo foo' would end up here.
      continue;
    }

    keyword->second.erase(node);
    if (keyword->second.empty())
      index_.erase(keyword);
  }

  favs_idx_.erase(i);
}

}  // namespace opera
