// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_FAVORITES_FAVORITE_COLLECTION_OBSERVER_H_
#define COMMON_FAVORITES_FAVORITE_COLLECTION_OBSERVER_H_

#include <string>

#include "common/favorites/favorite_collection.h"

namespace opera {

/**
 * A FavoriteCollectionObserver is notified of changes to the
 * FavoriteCollection.
 */
class FavoriteCollectionObserver {
 public:
  virtual ~FavoriteCollectionObserver() {}

  /**
   * Called when the Collection has finished loading.
   */
  virtual void FavoriteCollectionLoaded() {}

  /**
   * Called when the Collection is deleted.
   * It is forbidden to call the collection from this method
   * and after this call.
   */
  virtual void FavoriteCollectionDeleted() {}

  /**
   * Invoked when a Favorite has been added.
   * @param favorite The favorite that was changed.
   * @param new_index The new index of the favorite.
   * @param reason The reason of addition that was provided by the source
   *        of modification.
   */
  virtual void FavoriteAdded(const Favorite* favorite,
                             int new_index,
                             FavoriteCollection::ChangeReason reason) {}

  /**
   * Invoked when a Favorite is about to be removed.
   *
   * @param favorite The Favorite that is going to be removed.
   * @param parent The parent of the @p favorite.
   * @param index The index of the @p favorite.
   * @param reason The reason of removal that was provided by the source
   *        of modification.
   */
  virtual void BeforeFavoriteRemove(const Favorite* favorite,
                                    const FavoriteFolder* parent,
                                    int index,
                                    FavoriteCollection::ChangeReason reason) {}

  /**
   * Invoked after a Favorite is removed.
   *
   * @param favorite_guid The guid of the removed favorite.
   * @param reason The reason of removal that was provided by the source
   *        of modification.
   */
  virtual void AfterFavoriteRemoved(const std::string& favorite_guid,
                                    FavoriteCollection::ChangeReason reason) {}

  /**
   * Invoked when a Favorite has moved.
   * Note that this might mean its old parent is also removed.
   *
   * @param favorite The favorite that was moved.
   * @param new_parent The new parent of the favorite, or NULL if it was moved
   *                   to the root level.
   * @param new_index The index of the favorite after the move.
   * @param reason The reason of move that was provided by the source
   *        of modification.
   */
  virtual void FavoriteMoved(const Favorite* favorite,
                             const FavoriteFolder* old_parent,
                             int old_index,
                             const FavoriteFolder* new_parent,
                             int new_index,
                             FavoriteCollection::ChangeReason reason) {}

  /**
   * Invoked when the data of a Favorite changes.
   *
   * @param favorite the Favorite that has changed, with the new data already
   *    applied
   * @param old_data the old Favorite data
   * @param reason the reason of the change
   */
  virtual void FavoriteChanged(const Favorite* favorite,
                               const FavoriteData& old_data,
                               FavoriteCollection::ChangeReason reason) {}
};

}  // namespace opera

#endif  // COMMON_FAVORITES_FAVORITE_COLLECTION_OBSERVER_H_
