// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/favorites/favorite_suggestion_provider.h"

#include <algorithm>
#include <vector>

#include "base/strings/utf_string_conversions.h"

#include "components/query_parser/snippet.h"

#include "common/favorites/favorite_collection.h"
#include "common/favorites/favorite_utils.h"

#include "common/suggestion/suggestion_callback.h"
#include "common/suggestion/suggestion_provider.h"
#include "common/suggestion/suggestion_tokens.h"

namespace opera {

namespace {

const int kBaseScore = 900;
const int kMaxScore = 1199;

}  // namespace

FavoriteSuggestionProvider::FavoriteSuggestionProvider(
    FavoriteCollection* collection)
    : collection_(collection) {
}

void FavoriteSuggestionProvider::Query(
    const SuggestionTokens& query,
    bool private_browsing,
    const std::string& type,
    const SuggestionCallback& callback) {

  std::vector<FavoriteUtils::TitleMatch> matches;
  collection_->GetFavoritesWithTitlesMatching(
      base::UTF8ToUTF16(query.phrase()), collection_->GetCount(), &matches);

  std::vector<SuggestionItem> suggestions;

  for (std::vector<FavoriteUtils::TitleMatch>::const_iterator
       match_it = matches.begin();
       match_it != matches.end();
       ++match_it) {
    const Favorite* favorite = match_it->node;
    suggestions.push_back(SuggestionItem(CalculateScore(*match_it),
                                         base::UTF16ToUTF8(favorite->title()),
                                         favorite->navigate_url().spec(),
                                         type));
  }

  callback.Run(suggestions);
}

void FavoriteSuggestionProvider::Cancel() {
}

/**
 * Calculates the score for a match.
 *
 * For each term matching within the favorite's title (as given by
 * TitleMatch::match_positions) calculate a 'factor', sum up those factors,
 * then use the sum to figure out a value between the base score and the
 * maximum score.
 *
 * The factor for each term is calculated based on:
 *
 *  1) how much of the favorite's title has been matched by the term:
 *       (term length / title length).
 *
 *     Example: Given a favorite title 'abcde fghijklm', with a title
 *              length of 14, and two different search terms, 'abcde' and
 *              'fghijklm', with term lengths of 5 and 8, respectively,
 *              'fghijklm' will score higher (with a partial factor of
 *              8/14 = 0.571) than 'abcde' (5/14 = 0.357).
 *
 *  2) where the term match occurs within the favorite's title, giving more
 *     points for matches that appear earlier in the title:
 *     ((title length - position of match start) / title_length).
 *
 *     Example: Given a favorite title of 'abcde fghijklm', with a title
 *              length of 14, and two different search terms, 'abcde' and
 *              'fghij', with start positions of 0 and 6, respectively,
 *              'abcde' will score higher (with a a partial factor of
 *              (14-0)/14 = 1.000) than 'fghij' (with a partial factor of
 *              (14-6)/14 = 0.571).
 *
 * Once all term factors have been calculated they are summed. The resulting
 * sum will never be greater than 1.0. This sum is then multiplied against
 * the scoring range available, which is 299. The 299 is calculated by
 * subtracting the minimum possible score, 900, from the maximum possible
 * score, 1199. This product, ranging from 0 to 299, is added to the minimum
 * possible score, 900, giving the score.
 */
int FavoriteSuggestionProvider::CalculateScore(
    const FavoriteUtils::TitleMatch& match) {
  size_t title_length = match.node->title().size();
  double scoring_factor = 0.0;

  for (SuggestionMatchPositions::const_iterator it =
       match.match_positions.begin();
       it != match.match_positions.end();
       ++it) {
    double term_length = it->second - it->first;
    scoring_factor += (term_length / title_length) *
        (title_length - it->first) / title_length;
  }

  int range_score = kMaxScore - kBaseScore;
  return std::min(kMaxScore,
                  static_cast<int>(scoring_factor * range_score + kBaseScore));
}


}  // namespace opera
