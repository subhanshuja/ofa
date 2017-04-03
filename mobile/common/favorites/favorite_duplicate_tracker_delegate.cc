// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/favorites/favorite_duplicate_tracker_delegate.h"

#include "components/bookmarks/browser/bookmark_node.h"

#include "mobile/common/favorites/favorites.h"

namespace opera {

bool FavoriteDuplicateTrackerDelegate::IsLocalFavoriteNode(
    const bookmarks::BookmarkNode* favorites_root,
    const bookmarks::BookmarkNode* node) const {
  return node != favorites_root && node->HasAncestor(favorites_root);
}

const bookmarks::BookmarkNode*
FavoriteDuplicateTrackerDelegate::GetLocalFavoritesRootNode(
    PrefService* prefs, bookmarks::BookmarkModel* model) const {
  return mobile::Favorites::instance()->local_root()->data();
}

base::string16 FavoriteDuplicateTrackerDelegate::GetDefaultFolderName() const {
  return base::string16();
}

}  // namespace opera
