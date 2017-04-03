// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_FAVORITES_FAVORITE_COLLECTION_H_
#define COMMON_FAVORITES_FAVORITE_COLLECTION_H_

#include <string>
#include <vector>

#include "base/containers/hash_tables.h"
#include "base/memory/ref_counted.h"

#include "common/favorites/favorite.h"

namespace opera {

class FavoriteCollectionObserver;

namespace FavoriteUtils {
struct TitleMatch;
}

class FavoriteCollection
    : public base::RefCounted<FavoriteCollection> {
 public:
  /**
   * The reason of any favorite collection modification.
   * It will be passed to observers in change notifications.
   */
  enum ChangeReason {
    kChangeReasonSync,       ///< Applying changes from the Sync server
    kChangeReasonUser,       ///< Direct user action
    kChangeReasonAutoupdate, ///< Change from the autoupdate server
    kChangeReasonOther
  };

  /**
   * Favorite collection partner content logic delegate interface.
   */
  class PartnerLogicDelegate {
   public:
    virtual ~PartnerLogicDelegate() {}

    void set_favorite_collection(FavoriteCollection* collection) {
      collection_ = collection;
    }

    /**
     * This method is called before a favorite item is added or moved to a
     * folder.
     * @param favorite The favorite that is added or moved.
     * @param parent The parent folder to which the favorite is added/moved.
     * @param reason Change reason.
     */
    virtual void AddOrMoveFavorite(Favorite* favorite,
                                   FavoriteFolder* parent,
                                   ChangeReason reason) {}

    /**
     * This method is called after the data of a favorite item has been changed
     * (but before any observers are notified of the change).
     * @param favorite The favorite that has been changed.
     * @param old_data The data as it was before the change.
     * @param reason Change reason.
     */
    virtual void HandleChange(Favorite* favorite,
                              const FavoriteData& old_data,
                              ChangeReason reason) {}

    /**
     * This method is called after a favorite has been added or moved, after
     * all the observers have been notified but before the change has been
     * recorded in the persistent storage.
     * @param favorite The favorite that has been added.
     * @param reason Change reason.
     */
    virtual void OnFavoriteAddedOrMoved(
        Favorite* favorite, ChangeReason reason) {}

   protected:
    FavoriteCollection* collection_;
  };

  static const int kIndexPositionLast;

  /**
   * Set the observer that will receive notifications for operations on the
   * FavoriteCollection.
   */
  virtual void AddObserver(FavoriteCollectionObserver* observer) = 0;

  /**
   * Remove observer.
   */
  virtual void RemoveObserver(FavoriteCollectionObserver* observer) = 0;

  /**
   * @return true if the collection is loaded.
   */
  virtual bool IsLoaded() const = 0;

 /**
   * Add the favorite site to a given folder and at the given position.
   *
   * @param url The url of the new FavoriteSite.
   * @param parent The parent folder. If NULL, Favorite will be added
                   at the top level.
   * @param index position at which the new Favorite site should be added in
            given folder or #kIndexPositionLast to add as a last item.
   * @param guid id of favorite extension. If an empty string is passed, a
   *        new guid will be generated.
   * @param reason The reason of addition.
   * @return pointer to a FavoriteSite or NULL in case collection
   *         was unable to add new favorite (ie: invalid index was given or
   *         a favorite with given non empty guid already exists).
   */
  virtual FavoriteSite* AddFavoriteSite(const GURL& url,
                                        const base::string16& name,
                                        FavoriteFolder* parent,
                                        int index,
                                        const std::string& guid,
                                        ChangeReason reason) = 0;

  /**
   * Add the favorite extension to a given folder at the given position.
   *
   * Use this method only for favorites for which there is no extension
   * installed, otherwise use the one above.
   * When the extension is installed it can be associated by calling
   * FavoriteExtension::AssociateWithExtension.
   *
   * @param extension_id The ID of the extension of this favorite.
   * @param state The state to set the new FavoriteExtension in.
   * @param parent The parent folder. If NULL, the Favorite will be added
   *               at the top level.
   * @param index The position at which the new FavoriteExtension should be
   *              added in the given folder or #kIndexPositionLast to add as
   *              the last item.
   * @param reason
   */
  virtual FavoriteExtension* AddFavoriteExtension(
      const std::string& extension_id,
      const std::string& extension,
      FavoriteExtensionState state,
      FavoriteFolder* parent,
      int index,
      ChangeReason reason) = 0;

  /**
   * Add the favorite partner content to a given folder and at the given
   * position.
   *
   * @param url The url of the new FavoritePartnerContent.
   * @param parent The parent folder. If NULL, Favorite will be added
   *               at the top level.
   * @param partner_id Partner ID.
   * @param redirect Use redirection URL when navigating to this content.
   * @param index position at which the new FavoritePartnerContent should be
            added in given folder or #kIndexPositionLast to add as a last item.
   * @param reason The reason of addition.
   * @return pointer to a FavoriteSite or NULL in case collection
   *         was unable to add new favorite (ie: invalid index was given or
   *         a favorite with given non empty guid already exists).
   */
  virtual FavoriteSite* AddFavoritePartnerContent(
      const GURL& url,
      const base::string16& name,
      FavoriteFolder* parent,
      const std::string& guid,
      const std::string& partner_id,
      bool redirect,
      const FavoriteKeywords& custom_keywords,
      int index,
      ChangeReason reason) = 0;

  /**
   * Add a favorite partner content to a given folder at a given position
   *
   * @param url The url of the new partner content favorite
   * @param name The name to be shown for the favorite
   * @param parent The parent folder. If NULL, the favorite will be added at
   *               the top level
   * @param guid id of the favorite. If an empty string is passed, a new guid
   *             will be generated
   * @param pushed_partner_activation_count The activation count since last
   *                                        USAGE ACK from the partner content
   *                                        server
   * @param pushed_partner_channel
   * @param pushed_partner_id The id used when the partner content object is
   *                          accessed from the pushed content servers
   * @param pushed_partner_checksum
   * @param index position at which the new favorite should be added in the
   *              given fgolder, or #kIndexPositionLast to add it as the last
   *              item
   * @param reason The reason for adding the favorite
   * @return pointer to a Favorite item or NULL in case the addition failed
   */
  virtual FavoriteSite* AddFavoritePartnerContent(
      const GURL& url,
      const base::string16& name,
      FavoriteFolder* parent,
      const std::string& guid,
      int pushed_partner_activation_count,
      int pushed_partner_channel,
      int pushed_partner_id,
      int pushed_partner_checksum,
      int index,
      ChangeReason reason) = 0;

  /**
   * Add the favorite folder at the given position.
   *
   * @param index The position at which the new folder should be added or
   *        #kIndexPositionLast to add at the end.
   * @param guid id of favorite extension. If an empty string is passed, a
   *        new guid will be generated.
   * @param reason The reason of addition.
   * @return pointer to a FavoriteFolder or NULL in case collection
   *         was unable to add new favorite (ie: invalid index was given or
   *         a favorite with given non empty guid already exists).
   */
  virtual FavoriteFolder* AddFavoriteFolder(const base::string16& name,
                                            int index,
                                            const std::string& guid,
                                            ChangeReason reason) = 0;

  /**
   * Adds a partner content folder. The only difference from a normal folder is
   * that the given group id will be used.

   * @param name The name of the folder
   * @param pushed_partner_group_id The group id which identifies this folder
   * @param index The position at which the new folder should be added or
   *        #kIndexPositionLast to add at the end.
   * @param guid id of favorite extension. If an empty string is passed, a
   *        new guid will be generated.
   * @param reason The reason of addition.
   * @return pointer to a FavoriteFolder or NULL in case collection
   *         was unable to add new favorite (ie: invalid index was given or
   *         a favorite with given non empty guid already exists).
   */
  virtual FavoriteFolder* AddPartnerContentFolder(const base::string16& name,
                                                  int pushed_partner_group_id,
                                                  int index,
                                                  const std::string& guid,
                                                  ChangeReason reason) = 0;

  /**
   * Add the favorite placeholder folder at the given position.
   *
   * @param name The name of the new folder.
   * @param index The position at which the new folder should be added or
   *        #kIndexPositionLast to add at the end.
   * @param guid id of favorite extension. If an empty string is passed, a
   *        new guid will be generated.
   * @param reason The reason of addition.
   * @return pointer to a FavoriteFolder or NULL in case collection
   *         was unable to add new favorite (ie: invalid index was given or
   *         a favorite with given non empty guid already exists).
   */
  virtual FavoriteFolder* AddFavoritePlaceholderFolder(
      const base::string16& name,
      int index,
      const std::string& guid,
      ChangeReason reason) = 0;

  /**
   * Remove the favorite specified as parameter.
   *
   * @param entry The favorite to be removed. If this is a favorite folder
   *        that is not empty, the children will be also removed (before
   *        removing the folder).
   * @param reason The reason of removal.
   * @return true if the favorite was successfully removed.
   */
  virtual void RemoveFavorite(Favorite* favorite, ChangeReason reason) = 0;

  /**
   * Move the Favorite to a new position.
   *
   * @param favorite The favorite that is moved.
   * @param new_parent The parent the favorite should be moved to, or NULL
   *        if the favorite is to be moved to the root folder.
   * @param new_index The index at which the favorite is to be inserted
   *        or #kIndexPositionLast to insert at the end.
   *        NOTE: If the favorite is moved within the same folder, this
   *        is the index as if the moved favorite is not on its old place
   *        anymore. Than makes the behavior consistent with the case of moving
   *        a favorite between folders.
   * @param reason The reason of moving.
   * @return true if Favorite was moved, false otherwise.
   */
  virtual bool MoveFavorite(Favorite* favorite,
                            FavoriteFolder* new_parent,
                            int new_index,
                            ChangeReason reason) = 0;

  /**
   * Move the Favorite to a new position.
   *
   * @param entry The favorite that is moved.
   * @param new_parent The parent the favorite should be moved to, or NULL
   *        if the favorite is to be moved to the root folder.
   * @param prev_guid The guid of the favorite that the moved one should be
   *        placed after or empty string if the moved one should be placed
   *        first in the @p new_parent.
   * @param reason The reason of moving.
   * @return true if Favorite was moved, false otherwise. In particular will
   *         fail if prev_guid is invalid or no favorite with such guid was
   *         found in the target folder (not counting @p favorite).
   */
  virtual bool MoveFavorite(Favorite* favorite,
                            FavoriteFolder* new_parent,
                            const std::string& prev_guid,
                            ChangeReason reason) = 0;

  /**
   * Changes the data of a Favorite.
   *
   * The changes are applied according to @p new_data.  However,
   * FavoriteCollection, as the owner of its Favorites, reserves the right to
   * perform certain post-processing on the changes, which in some cases results
   * in other changes applied to @p favorite or even other Favorite objects in
   * this collection.
   *
   * All changes made through this function trigger the
   * FavoriteCollectionObserver::FavoriteChanged() notification for every
   * Favorite object that is changed.
   *
   * @param favorite the favorite to be changed
   * @param new_data the data to apply to @p favorite
   * @param reason the reason of the change
   */
  virtual void ChangeFavorite(Favorite* favorite,
                              const FavoriteData& new_data,
                              ChangeReason reason) = 0;

  /**
   * Returns the set of favorites having a given url.
   *
   * FavoriteExtensions are excluded.
   *
   * @param url The url to search for
   * @param favorites A vector of favorites to be filled with the
   *                  favorites having the requested URL. The
   *                  FavoriteCollection retains ownership of the
   *                  favorites
   */
  virtual void GetFavoritesByURL(const GURL& url,
                                 std::vector<const Favorite*>* favorites) = 0;

  /**
   * Get the number of favorites present at root level.
   *
   * @return The count of favorites at the root level.
   */
  virtual int GetCount() const = 0;

  /**
   * @return the Favorite at @p index index.
   */
  virtual Favorite* GetAt(int index) const = 0;

  /**
   * Get the index of a favorite at root level.
   * @param A root level favorite
   * @return The index of favorite. -1 if not found at root level.
   */
  virtual int GetIndexOf(const Favorite* favorite) const = 0;

  /**
   * @param guid The GUID of the Favorite that is to be found.
   *
   * @return The Favorite matching given @p guid or NULL.
   */
  virtual Favorite* FindByGUID(const std::string& guid) const = 0;

  /**
   * Returns up to @p max_count of favorites containing the text @p query.
  */
  virtual void GetFavoritesWithTitlesMatching(
      const base::string16& query,
      size_t max_count,
      std::vector<FavoriteUtils::TitleMatch>* matches) = 0;

  /**
   *  Remove all the FavoritePartnerContent elements with the exception of
   *  elements in the to_stay container.
   */
  virtual void FilterFavoritePartnerContent(
      const base::hash_set<std::string>& to_stay) = 0;

  virtual FavoriteFolder* GetRoot() const = 0;

  /**
   * Store any unsaved data to disk before returning.
   */
  virtual void Flush() = 0;

 protected:
  friend class base::RefCounted<FavoriteCollection>;
  virtual ~FavoriteCollection() {}
};

}  // namespace opera

#endif  // COMMON_FAVORITES_FAVORITE_COLLECTION_H_
