// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_FAVORITES_FAVORITE_H_
#define COMMON_FAVORITES_FAVORITE_H_

#include <list>
#include <string>

#include "base/compiler_specific.h"
#include "base/strings/string16.h"
#include "url/gurl.h"

#include "common/favorites/favorite_data.h"

namespace opera {

class Favorite;
class FavoriteFolder;

class Favorite : public FavoriteDataHolder {
 public:
  enum Type {
    kFavoriteSite,
    kFavoriteFolder,
    kFavoriteExtension,
  };

  virtual ~Favorite() {}

  const std::string& guid() const { return guid_; }

  const base::string16& title() const { return data().title; }

  /**
   * @return the URL to use when navigating to the site represented by this
   *    Favorite
   */
  const GURL& navigate_url() const;

  /**
   * @return the URL to be shown to the user in the UI
   */
  const GURL& display_url() const { return data().url; }

  const std::string& partner_id() const { return data().partner_id; }

  /**
   * Gets the keywords associated with this Favorite.
   *
   * The keywords are used by suggestions when finding the set of favorites that
   * match a query.
   *
   * Some keywords are deduced from the Favorite title.  The keywords set in
   * FavoriteData::custom_keywords, if any, are included in the result.
   */
  FavoriteKeywords GetKeywords() const;

  /**
   * @see FavoriteData::is_placeholder_folder
   */
  bool is_placeholder_folder() const { return data().is_placeholder_folder; }

  /**
   * @return the parent folder or NULL if this is a root folder
   */
  FavoriteFolder* parent() const { return parent_; }

  virtual Favorite::Type GetType() const = 0;

  /**
   * Whether this favorite's URL should be included for suggestions and
   * used by the "add to speed dial" icon.
   */
  virtual bool AllowURLIndexing() const = 0;

  /**
   * Whether the favorite is transient.
   *
   * Transient favorites are not saved to storage.
   */
  virtual bool IsTransient() const;

  int pushed_partner_activation_count() const {
    return data().pushed_partner_activation_count;
  }
  int pushed_partner_channel() const { return data().pushed_partner_channel; }
  int pushed_partner_id() const { return data().pushed_partner_id; }
  int pushed_partner_group_id() const { return data().pushed_partner_group_id; }
  int pushed_partner_checksum() const { return data().pushed_partner_checksum; }

 protected:
  Favorite(const std::string& guid, const FavoriteData& data);

 private:
  friend class FavoriteFolder;
  friend class FavoriteRootFolder;

  void set_parent(FavoriteFolder* parent);

  const std::string guid_;

  FavoriteFolder* parent_;
};

/**
 * Favorite Site
 *
 * A Favorite Site represented by an URL shown in UI with thumbnail
 * representation.
 */
class FavoriteSite : public Favorite {
 public:
  FavoriteSite(const std::string& guid, const FavoriteData& data);
  virtual ~FavoriteSite();

  // Favorite overrides:
  Favorite::Type GetType() const override;
  bool AllowURLIndexing() const override;
};

/**
 * A Folder grouping a set of favorites in the UI.
 */
class FavoriteFolder : public Favorite {
 public:
  typedef std::list<Favorite*>::const_iterator const_iterator;

  FavoriteFolder(const std::string& guid, const FavoriteData& data);
  virtual ~FavoriteFolder();

  const_iterator begin() const;
  const_iterator end() const;

  /**
   * @return number of children of this FavoriteFolder.
   */
  int GetChildCount() const;

  /**
   * @return the child at index @p index, if any.
   */
  Favorite* GetChildByIndex(int index) const;

  /**
   * Get the index of given favorite.
   * @return index of favorite or -1 if such favorite does not exist in
   *         this FavoriteFolder.
   */
  int GetIndexOf(const Favorite* favorite) const;

  /**
   * Find the Favorite in the current folder.
   *
   * @return The found Favorite or NULL.
   */
  Favorite* FindByGUID(const std::string& guid) const;

  // Favorite overrides:
  Favorite::Type GetType() const override;
  bool AllowURLIndexing() const override;

 protected:
  friend class FavoriteRootFolder;

  /**
   * Add child at given index.
   *
   * NOTE: subfolders are not allowed but for purposes of current implementation
   *       we allow FavoriteCollection's root folder to have subfolders.
   * The index must be in the [0, child count] interval.
   */
  void AddChildAt(Favorite* child, int index);

  /**
   * Remove child favorite.
   *
   * NOTE: Doesn't delete @p child.
   */
  void RemoveChild(Favorite* child);

  /**
   * Move child to another FavoriteFolder.
   * Both src_index and dest_index must be valid, so that
   * moving can't fail.
   */
  void MoveChildToFolder(Favorite* child,
                         FavoriteFolder* dest_folder,
                         int src_index,
                         int dest_index);

 private:
  typedef std::list<Favorite*> FavoriteEntries;

  /**
   * Child favorites.
   */
  FavoriteEntries children_;
};


class FavoriteExtension : public Favorite {
 public:
  /**
   * @param transient determines if this favorite should not be saved to
   *    storage
   */
  FavoriteExtension(const std::string& guid,
                    const FavoriteData& data,
                    bool transient);

  virtual ~FavoriteExtension();

  const std::string& extension() const {
    return data().extension;
  }

  const std::string& extension_id() const {
    return data().extension_id;
  }

  FavoriteExtensionState state() const { return data().extension_state; }

  const base::string16& live_title() const { return data().live_title; }
  const GURL& live_url() const { return data().live_url; }

  // Favorite overrides:
  Favorite::Type GetType() const override;
  bool AllowURLIndexing() const override;
  bool IsTransient() const override;

 private:
  /**
   * The transient property of this favorite determining
   * if it should not be saved to storage.
   */
  bool transient_;
};

}  // namespace opera

#endif  // COMMON_FAVORITES_FAVORITE_H_
