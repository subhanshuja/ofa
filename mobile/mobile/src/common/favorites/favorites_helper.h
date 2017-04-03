// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_FAVORITES_HELPER_H_
#define COMMON_SYNC_FAVORITES_HELPER_H_

namespace bookmarks {
class BookmarkNode;
}

namespace opera {
class SuggestionProvider;
}  // namespace opera

namespace mobile {

class FavoritesHelper {
 public:
  static void ImportFromCollection();
  static void SetBookmarksEnabled(bool enabled);
  static void SetSavedPagesDisabled();

  // Returns the "speed_dial" node that is local to this device.
  static const bookmarks::BookmarkNode* GetLocalSpeedDialNode();

  static opera::SuggestionProvider* CreateSuggestionProvider();

 private:
  FavoritesHelper() {}
  ~FavoritesHelper() {}
};

}  // namespace mobile

#endif  // COMMON_SYNC_FAVORITES_HELPER_H_
