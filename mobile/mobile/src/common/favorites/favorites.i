// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "mobile/common/favorites/favorites.h"
#include "mobile/common/favorites/favorites_observer.h"
#include "common/favorites/favorites_helper.h"
%}

%typemap(javacode) mobile::Favorite %{
  static Favorite CreateFromCPtr(long cPtr, boolean cMemoryOwn) {
    if (cPtr == 0)
      return null;
    if (CPtrIsFolder(cPtr))
      return new Folder(cPtr, cMemoryOwn);
    else if (CPtrIsSavedPage(cPtr))
      return new SavedPage(cPtr, cMemoryOwn);
    else
      return new Favorite(cPtr, cMemoryOwn);
  }
%}

%typemap(javaout) mobile::Favorite* {
    return Favorite.CreateFromCPtr($jnicall, $owner);
  }

%typemap(javadirectorin) mobile::Favorite* "Favorite.CreateFromCPtr($jniinput, false)"

namespace mobile {

%nodefaultctor Favorite;
%nodefaultdtor Favorite;
class Favorite {
 public:
  bool IsFolder() const;
  bool IsSavedPage() const;
  bool CanChangeTitle() const;
  bool CanTransformToFolder() const;
  bool CanChangeParent() const;
  bool IsPartnerContent() const;

  int64_t id() const;
  int64_t parent() const;

  const base::string16& title() const;
  void SetTitle(const base::string16& title);

  const GURL& url() const;
  void SetURL(const GURL& url);

  const std::string& thumbnail_path() const;

  void Activated();

  bool HasAncestor(int64_t id) const;
};

%extend Favorite {
  static bool CPtrIsFolder(jlong cPtr) {
    mobile::Favorite* favorite = (mobile::Favorite*) cPtr;
    return favorite->IsFolder();
  }
  static bool CPtrIsSavedPage(jlong cPtr) {
    mobile::Favorite* favorite = (mobile::Favorite*) cPtr;
    return favorite->IsSavedPage();
  }
};

%nodefaultctor Folder;
%nodefaultdtor Folder;
class Folder : public Favorite {
 public:
  bool CanTakeMoreChildren() const;

  int Size() const;

  void Add(Favorite* favorite);
  void Add(int pos, Favorite* favorite);

  void AddAll(Folder* folder);

  int IndexOf(Favorite* favorite);
  int IndexOf(int64_t id);

  const base::Time& last_modified() const;

  Favorite* Child(int index);
};

%nodefaultctor SavedPage;
%nodefaultdtor SavedPage;
class SavedPage : public Favorite {
 public:
  const std::string& file() const;
  void SetFile(const std::string& file);
};

class FavoritesObserver;

%nodefaultctor Favorites;
%nodefaultdtor Favorites;
class Favorites {
 public:
  static Favorites* instance();

  bool IsReady();
  bool IsLoaded();

  Folder* devices_root();
  Folder* local_root();
  Folder* bookmarks_folder();
  Folder* saved_pages();

  void SetBookmarksFolderTitle(const base::string16& title);
  void SetSavedPagesTitle(const base::string16& title);
  void SetRequestGraphicsSizes(int icon_size, int thumb_size);
  void SetBaseDirectory(const std::string& path);
  void SetSavedPageDirectory(const std::string& path);

  Folder* CreateFolder(int pos, const base::string16& title);
  Folder* CreateFolder(
      int pos, const base::string16& title, int pushed_partner_group_id);
  Favorite* CreateFavorite(Folder* parent, int pos, const base::string16& title,
                           const GURL& url);
  SavedPage* CreateSavedPage(const base::string16& title, const GURL& url,
                             const std::string& file);

  void Remove(int64_t id);
  void ClearThumbnail(int64_t id);

  Favorite* GetFavorite(int64_t id);

  void AddObserver(FavoritesObserver* observer);
  void RemoveObserver(FavoritesObserver* observer);

  void Flush();

  bool IsLocal(int64_t id);
};

%nodefaultctor FavoritesHelper;
%nodefaultdtor FavoritesHelper;
class FavoritesHelper {
 public:
  static void ImportFromCollection();
  static const bookmarks::BookmarkNode* GetLocalSpeedDialNode();
  static void SetBookmarksEnabled(bool enabled);
  static void SetSavedPagesDisabled();

  %newobject CreateSuggestionProvider;
  static opera::SuggestionProvider* CreateSuggestionProvider();
};

%feature("director", assumeoverride=1) FavoritesObserver;

}

%include "../../../common/favorites/favorites_observer.h"
