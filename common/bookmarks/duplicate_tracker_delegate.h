// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_BOOKMARKS_DUPLICATE_TRACKER_DELEGATE_H_
#define COMMON_BOOKMARKS_DUPLICATE_TRACKER_DELEGATE_H_

#include "base/strings/string16.h"

class PrefService;

namespace bookmarks {
class BookmarkModel;
class BookmarkNode;
}

namespace opera {

class DuplicateTrackerDelegate {
 public:
  virtual ~DuplicateTrackerDelegate() = default;

  virtual bool IsLocalFavoriteNode(
      const bookmarks::BookmarkNode* favorites_root,
      const bookmarks::BookmarkNode* node) const = 0;

  virtual const bookmarks::BookmarkNode* GetLocalFavoritesRootNode(
      PrefService* prefs,
      bookmarks::BookmarkModel* model) const = 0;

  virtual base::string16 GetDefaultFolderName() const = 0;
};

}  // namespace opera

#endif  // COMMON_BOOKMARKS_DUPLICATE_TRACKER_DELEGATE_H_
