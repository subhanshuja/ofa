// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/browser/bookmarks/bookmark_index.h
// @final-synchronized bdec20a2da9cf437cd0e52ee57c9572eddcfa02e

#ifndef COMMON_FAVORITES_FAVORITE_INDEX_H_
#define COMMON_FAVORITES_FAVORITE_INDEX_H_

#include <map>
#include <set>
#include <utility>
#include <vector>

#include "base/strings/string16.h"
#include "url/gurl.h"

namespace opera {
class Favorite;
namespace query_parser {
class QueryNode;
class QueryParser;
}
namespace FavoriteUtils {
struct TitleMatch;
}  // namespace FavoriteUtils

class FavoriteIndex {
 public:
  explicit FavoriteIndex();
  ~FavoriteIndex();

  // Invoked when a favorite has been added to the model.
  void Add(const Favorite* fav);

  // Invoked when a favorite has been removed from the model.
  void Remove(const Favorite* fav);

  // Returns up to |max_count| of favorites containing the text |query|.
  void GetFavoritesWithTitlesMatching(
      const base::string16& query,
      size_t max_count,
      std::vector<FavoriteUtils::TitleMatch>* results);

 private:
  typedef std::set<const Favorite*> NodeSet;
  typedef std::set<base::string16> KeywordsSet;
  typedef std::map<base::string16, NodeSet> Index;
  typedef std::map<const Favorite*, KeywordsSet> FavsIdx;

  struct Match;
  typedef std::vector<Match> Matches;

  // Pairs Favorite and the number of times the nodes' URLs were typed.
  // Used to sort Matches in decreasing order of typed count.
  typedef std::pair<const Favorite*, int> NodeTypedCountPair;
  typedef std::vector<NodeTypedCountPair> NodeTypedCountPairs;

  // Extracts |matches.nodes| into NodeTypedCountPairs, sorts the pairs in
  // decreasing order of typed count, and then de-dupes the matches.
  void SortMatches(const Matches& matches,
                   NodeTypedCountPairs* node_typed_counts) const;

  // Extracts FavoriteNodes from |match| and retrieves typed counts for each
  // node from the in-memory database. Inserts pairs containing the node and
  // typed count into the vector |node_typed_counts|. |node_typed_counts| is
  // sorted in decreasing order of typed count.
  void ExtractFavoriteNodePairs(const Match& match,
                                NodeTypedCountPairs* node_typed_counts) const;

  // Sort function for NodeTypedCountPairs. We sort in decreasing order of typed
  // count so that the best matches will always be added to the results.
  static bool NodeTypedCountPairSortFunc(const NodeTypedCountPair& a,
                                         const NodeTypedCountPair& b) {
      return a.second > b.second;
  }

  // Add |node| to |results| if the node matches the query.
  void AddMatchToResults(
      const Favorite* fav,
      query_parser::QueryParser* parser,
      const std::vector<query_parser::QueryNode*>& query_nodes,
      std::vector<opera::FavoriteUtils::TitleMatch>* results);

  // Populates |matches| for the specified term. If |first_term| is true, this
  // is the first term in the query. Returns true if there is at least one node
  // matching the term.
  bool GetFavoritesWithTitleMatchingTerm(const base::string16& term,
                                         bool first_term,
                                         Matches* matches);

  // Iterates over |matches| updating each Match's nodes to contain the
  // intersection of the Match's current nodes and the nodes at |index_i|.
  // If the intersection is empty, the Match is removed.
  //
  // This is invoked from GetFavoritesWithTitleMatchingTerm.
  void CombineMatchesInPlace(const Index::const_iterator& index_i,
                             Matches* matches);

  // Iterates over |current_matches| calculating the intersection between the
  // Match's nodes and the nodes at |index_i|. If the intersection between the
  // two is non-empty, a new match is added to |result|.
  //
  // This differs from CombineMatchesInPlace in that if the intersection is
  // non-empty the result is added to result, not combined in place. This
  // variant is used for prefix matching.
  //
  // This is invoked from GetFavoritesWithTitleMatchingTerm.
  void CombineMatches(const Index::const_iterator& index_i,
                      const Matches& current_matches,
                      Matches* result);

  // Returns the set of query words from |query|.
  std::vector<base::string16> ExtractQueryWords(const base::string16& query);

  // Adds |node| to |index_|.
  void RegisterNode(const base::string16& term, const Favorite* fav);

  // Removes |node| from |index_|.
  void UnregisterNode(const Favorite* fav);

  Index index_;
  FavsIdx favs_idx_;

  DISALLOW_COPY_AND_ASSIGN(FavoriteIndex);
};
}  // namespace opera
#endif  // COMMON_FAVORITES_FAVORITE_INDEX_H_
