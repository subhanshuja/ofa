// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_FAVORITES_FAVORITE_UTILS_H_
#define COMMON_FAVORITES_FAVORITE_UTILS_H_

#include "base/callback.h"
#include "common/suggestion/snippet.h"

namespace opera {

class Favorite;
class FavoriteFolder;

namespace FavoriteUtils {
/**
 * Used by GetFavoritesMatchingText to return a matching node and the location
 * of the match in the title.
 */
struct TitleMatch {
  TitleMatch();
  ~TitleMatch();

  const Favorite* node;

  // Location of the matching words in the title of the node.
  SuggestionMatchPositions match_positions;
};

void ForEachFavoriteInFolder(
    const opera::FavoriteFolder* folder,
    bool recurse,
    const base::Callback<void(opera::Favorite*, int favorite_index)>& task);

}  // namespace FavoriteUtils
}  // namespace opera

#endif  // COMMON_FAVORITES_FAVORITE_UTILS_H_
