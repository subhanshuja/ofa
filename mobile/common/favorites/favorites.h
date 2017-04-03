// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_FAVORITES_FAVORITES_H_
#define MOBILE_COMMON_FAVORITES_FAVORITES_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "base/single_thread_task_runner.h"

#include "mobile/common/favorites/favorite.h"
#include "mobile/common/favorites/favorites_delegate.h"
#include "mobile/common/favorites/favorites_observer.h"
#include "mobile/common/favorites/folder.h"
#include "mobile/common/favorites/savedpage.h"

namespace bookmarks {
class BookmarkModel;
}

namespace opera {
class FavoriteCollection;
class SuggestionProvider;
}

namespace mobile {

class FavoritesDelegate;
struct TitleMatch;

class Favorites {
 public:
  enum SavedPagesMode {
    ENABLED,  // Visible if there are any saved pages. saved_pages() returns
              // null if the folder isn't visible
    DISABLED, // Never visible, saved_pages() will still return the folder
              // if there are any saved pages. The returned folder will not
              // be a sub folder of local_root() but a root folder itself
    ALWAYS,   // Always visible and saved_pages() never returns null
  };

  virtual ~Favorites() {}

  static Favorites* instance();

  virtual bool IsReady() = 0;
  virtual bool IsLoaded() = 0;

  // Root for all synced devices + the local device, each child is a folder
  // with the name of the device
  virtual Folder* devices_root() = 0;

  // Child of devices_root, always points to the local device
  virtual Folder* local_root() = 0;

  // May return NULL if no such folder exists at this time
  virtual Folder* bookmarks_folder() = 0;

  // May return NULL if no such folder exists at this time
  // The returned folder might be a root folder (have itself as parent)
  // and thus be disconnected from the local_root() if SavedPagesMode is
  // DISABLED
  virtual Folder* saved_pages() = 0;

  // May return NULL if no such folder exists at this time
  virtual Folder* feeds() = 0;

  virtual void SetBookmarksFolderTitle(const base::string16& title) = 0;

  virtual void SetSavedPagesTitle(const base::string16& title) = 0;

  virtual void SetFeedsTitle(const base::string16& title) = 0;

  virtual void SetRequestGraphicsSizes(int icon_size, int thumb_size) = 0;

  virtual void SetBaseDirectory(const std::string& path) = 0;

  virtual void SetSavedPageDirectory(const std::string& path) = 0;

  virtual void SetDelegate(FavoritesDelegate* delegate) = 0;

  virtual void SetModel(
      bookmarks::BookmarkModel* bookmark_model,
      const scoped_refptr<base::SingleThreadTaskRunner>& ui_task_runner,
      const scoped_refptr<base::SingleThreadTaskRunner>& file_task_runner) = 0;

  virtual void SetDeviceName(const std::string& name) = 0;

  virtual void SetBookmarksEnabled(bool enabled) = 0;

  // This must be called before favorites is loaded, default is ENABLED
  virtual void SetSavedPagesEnabled(SavedPagesMode mode) = 0;

  virtual void SetFeedsEnabled(bool enabled) = 0;

  virtual Folder* CreateFolder(int pos, const base::string16& title) = 0;
  virtual Folder* CreateFolder(int pos,
                               const base::string16& title,
                               int pushed_partner_group_id) = 0;
  virtual Favorite* CreateFavorite(Folder* parent,
                                   int pos,
                                   const base::string16& title,
                                   const GURL& url) = 0;
  virtual SavedPage* CreateSavedPage(const base::string16& title,
                                     const GURL& url,
                                     const std::string& file) = 0;

  virtual void Remove(Favorite* favorite);
  virtual void ClearThumbnail(Favorite* favorite);

  virtual void Remove(int64_t id) = 0;
  virtual void ClearThumbnail(int64_t id) = 0;

  // May return NULL if id no longer exists (id:s may however be reused)
  virtual Favorite* GetFavorite(int64_t id) = 0;

  virtual void AddObserver(FavoritesObserver* observer) = 0;

  virtual void RemoveObserver(FavoritesObserver* observer) = 0;

  virtual std::unique_ptr<opera::SuggestionProvider>
      CreateSuggestionProvider() = 0;

  // Import favorites from FavoriteCollection
  virtual void ImportFromFavoriteCollection(
      const scoped_refptr<opera::FavoriteCollection>& collection) = 0;

  // Store any unsaved data to disk
  virtual void Flush() = 0;

  virtual void GetFavoritesMatching(const std::string& query,
                                    size_t max_matches,
                                    std::vector<TitleMatch>* matches) = 0;

  virtual void GetFavoritesByURL(const GURL& url,
                                 std::vector<int64_t>* matches) = 0;

  // Call to force favorites to modify the root folder so it wont be merged
  // at next sync. This is currently implemented by changing GUID on the root
  // folder and remembering the old GUID to remove upon next sync as cleanup
  virtual void ProtectRootFromSyncMerge() = 0;

  // Short for GetFavorite(id)->HasAnscestor(local_root_)
  virtual bool IsLocal(int64_t id);

  virtual void RemoveRemoteDevices() = 0;

 protected:
  Favorites();

 private:
  DISALLOW_COPY_AND_ASSIGN(Favorites);
};

}  // namespace mobile

#endif  // MOBILE_COMMON_FAVORITES_FAVORITES_H_
