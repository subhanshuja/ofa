// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/favorites/favorites_helper.h"

#include "base/android/path_utils.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "content/public/browser/browser_thread.h"

#include "common/favorites/favorite_collection_impl.h"
#include "common/favorites/favorite_db_storage.h"
#include "common/suggestion/suggestion_provider.h"
#include "mobile/common/favorites/favorites.h"

namespace mobile {

// static
void FavoritesHelper::ImportFromCollection() {
  base::FilePath app_base_path;
  PathService::Get(base::DIR_ANDROID_APP_DATA, &app_base_path);

  // Create the Favorite collection.
  opera::FavoriteStorage* favorite_storage = new opera::FavoriteDBStorage(
      app_base_path,
      content::BrowserThread::GetTaskRunnerForThread(
          content::BrowserThread::DB));
  scoped_refptr<opera::FavoriteCollection> favorite_collection =
      new opera::FavoriteCollectionImpl(
          content::BrowserThread::GetTaskRunnerForThread(
              content::BrowserThread::UI),
          favorite_storage,
          NULL);

  Favorites::instance()->ImportFromFavoriteCollection(favorite_collection);
}

// static
const bookmarks::BookmarkNode* FavoritesHelper::GetLocalSpeedDialNode() {
  if (Favorites::instance()->IsReady()) {
    return Favorites::instance()->local_root()->data();
  }
  // Should not happen. Make sure Favorites instance is ready before calling.
  NOTREACHED();
  return nullptr;
}

void FavoritesHelper::SetBookmarksEnabled(bool enabled) {
  Favorites::instance()->SetBookmarksEnabled(enabled);
}

void FavoritesHelper::SetSavedPagesDisabled() {
  Favorites::instance()->SetSavedPagesEnabled(mobile::Favorites::DISABLED);
}

opera::SuggestionProvider* FavoritesHelper::CreateSuggestionProvider() {
  return Favorites::instance()->CreateSuggestionProvider().release();
}

}  // namespace mobile
