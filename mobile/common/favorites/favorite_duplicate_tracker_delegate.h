// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_FAVORITES_FAVORITE_DUPLICATE_TRACKER_DELEGATE_H_
#define MOBILE_COMMON_FAVORITES_FAVORITE_DUPLICATE_TRACKER_DELEGATE_H_

#include "common/bookmarks/duplicate_tracker_delegate.h"

namespace opera {

class FavoriteDuplicateTrackerDelegate : public DuplicateTrackerDelegate {
 public:
  ~FavoriteDuplicateTrackerDelegate() override = default;

  bool IsLocalFavoriteNode(const bookmarks::BookmarkNode* favorites_root,
                           const bookmarks::BookmarkNode* node) const override;

  const bookmarks::BookmarkNode* GetLocalFavoritesRootNode(
      PrefService* prefs,
      bookmarks::BookmarkModel* model) const override;

  base::string16 GetDefaultFolderName() const override;
};

}  // namespace opera

#endif  // MOBILE_COMMON_FAVORITES_FAVORITE_DUPLICATE_TRACKER_DELEGATE_H_
