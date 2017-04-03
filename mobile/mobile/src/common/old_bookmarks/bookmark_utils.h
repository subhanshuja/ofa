// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013-2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_UTILS_H_
#define MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_UTILS_H_

#include "base/callback.h"

#include "common/suggestion/snippet.h"

namespace opera {

class Bookmark;
class BookmarkFolder;

namespace BookmarkUtils {

/**
 * Used by GetFavoritesMatchingText to return a matching node and the location
 * of the match in the title.
 */
struct TitleMatch {
  TitleMatch();
  ~TitleMatch();

  const Bookmark* node;

  // Location of the matching words in the title of the node.
  SuggestionMatchPositions match_positions;
};

void ForEachBookmarkInFolder(
    const opera::BookmarkFolder* folder,
    bool recurse,
    const base::Callback<void(opera::Bookmark*, int bookmark_index)>& task);

int GetTotalCount(const opera::BookmarkFolder* folder);

// Count the number of folders recursively
int GetTotalFoldersCount(const opera::BookmarkFolder* folder);

}  // namespace BookmarkUtils
}  // namespace opera

#endif  // MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_UTILS_H_
