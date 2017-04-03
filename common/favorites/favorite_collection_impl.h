// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_FAVORITES_FAVORITE_COLLECTION_IMPL_H_
#define COMMON_FAVORITES_FAVORITE_COLLECTION_IMPL_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/containers/hash_tables.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"

#include "common/favorites/favorite.h"
#include "common/favorites/favorite_collection.h"
#include "common/favorites/favorite_index.h"
#include "common/favorites/favorite_storage.h"

class GURL;

namespace opera {

/**
 * Favorite Root Folder.
 *
 * Implements local favorite storage reusing existing FavoriteFolder
 * implementation.
 */
class FavoriteRootFolder : public FavoriteFolder {
 public:
  FavoriteRootFolder();

  /**
   * Checks if a favorite can be added at @p index in the
   * @p parent. @p index might be @c FavoriteCollection::kIndexPositionLast.
   */
  bool CanAddChildAt(FavoriteFolder* parent, int index);

  /**
   * Add @p child to @p parent at @p index.
   *
   * @param index the index where the child is added.  If it's
   *    FavoriteCollection::kIndexPositionLast, it receives the actual index
   *    where the child is added.
   */
  void AddChildAt(FavoriteFolder* parent, Favorite* child, int* index);

  void AddChildAt(FavoriteFolder* parent, Favorite* child, int index) {
    AddChildAt(parent, child, &index);
  }

  /**
   * Remove @p favorite.
   */
  void Remove(Favorite* favorite);

  /**
   * Checks if a @p favorite can be moved to @p target_folder
   * to the @p target_index.
   */
  bool CanMove(Favorite* favorite,
               FavoriteFolder* target_folder,
               int target_index);

  /**
   * Move @p favorite to @p move_to_folder at @p move_to_index new position.
   */
  void Move(Favorite* favorite,
            FavoriteFolder* move_to_folder,
            int move_from_index,
            int move_to_index);

  /**
   * Checks if @p favorite can be moved to @p new_parent after
   * a favorite with @p prev_guid.
   * Empty @p prev_guid means place first in new_parent.
   */
  bool CanMove(Favorite* favorite,
               FavoriteFolder* new_parent,
               const std::string& prev_guid);

  /**
   * Move a favorite.
   *
   * Assumes that the params make the operation possible.
   *
   * @param favorite A favorite to move.
   * @param new_parent A folder to move to. NULL means root folder.
   * @param prev_guid A favorite to move after. Empty prev_guid means place
   *        as first in @p move_to_folder.
   * @param[out] new_index Cannot be NULL. If the operation is successful,
   *             the new index of moved favorite will be assigned.
   */
  void Move(Favorite* favorite,
            FavoriteFolder* new_parent,
            const std::string& prev_guid,
            int* new_index);

  /**
   * Get index of @p favorite relative to its parent.
   */
  int GetIndexOf(const Favorite* favorite) const;

  /**
   * Get top level child count.
   */
  int GetChildCount() const;

  /**
   * Get child at @p index.
   */
  Favorite* GetChildByIndex(int index) const;

  /**
   * Recreate favorite from given data. No notification is being sent
   * when recreating favorites. This is used to recreate local favorite
   * storage from data kept in persistent storage.
   */
  bool RecreateFavorite(const std::string& guid,
                        const base::string16& name,
                        const std::string& url,
                        int type,
                        const std::string& parent_guid,
                        const base::Pickle& serialized_data);

 private:
  /**
   * Find element by given @p guid. Searches top level only.
   */
  FavoriteFolder* FindByGUID(const std::string& guid) const;

  /*
   * Returns real parent for @favorite.
   * When Favorite's parent is NULL pointer to internal root
   * folder will be returned.
   */
  FavoriteFolder* GetRealParent(const Favorite* favorite);
  const FavoriteFolder* GetRealParent(const Favorite* favorite) const;
};

class FavoriteCollectionImpl
    : public FavoriteCollection {
 public:
  FavoriteCollectionImpl(scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    FavoriteStorage* storage,
    std::unique_ptr<PartnerLogicDelegate> partner_logic_delegate);

  // FavoriteCollection implementation:
  void AddObserver(FavoriteCollectionObserver* observer) override;
  void RemoveObserver(FavoriteCollectionObserver* observer) override;
  bool IsLoaded() const override;
  virtual FavoriteSite* AddFavoriteSite(const GURL& url,
                                        const base::string16& name,
                                        FavoriteFolder* parent,
                                        int index,
                                        const std::string& guid,
                                        ChangeReason reason) override;
  virtual FavoriteExtension* AddFavoriteExtension(
      const std::string& extension_id,
      const std::string& extension,
      FavoriteExtensionState state,
      FavoriteFolder* parent,
      int index,
      ChangeReason reason) override;
  virtual FavoriteSite* AddFavoritePartnerContent(
      const GURL& url,
      const base::string16& name,
      FavoriteFolder* parent,
      const std::string& guid,
      const std::string& partner_id,
      bool redirect,
      const FavoriteKeywords& custom_keywords,
      int index,
      ChangeReason reason) override;
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
      ChangeReason reason) override;
  virtual FavoriteFolder* AddFavoriteFolder(const base::string16& name,
                                            int index,
                                            const std::string& guid,
                                            ChangeReason reason) override;
  virtual FavoriteFolder* AddPartnerContentFolder(const base::string16& name,
                                                  int pushed_partner_group_id,
                                                  int index,
                                                  const std::string& guid,
                                                  ChangeReason reason) override;
  virtual FavoriteFolder* AddFavoritePlaceholderFolder(
      const base::string16& name,
      int index,
      const std::string& guid,
      ChangeReason reason) override;

  virtual void RemoveFavorite(Favorite* favorite,
                              ChangeReason reason) override;
  virtual bool MoveFavorite(Favorite* favorite,
                            FavoriteFolder* new_parent,
                            int new_index,
                            ChangeReason reason) override;
  virtual bool MoveFavorite(Favorite* favorite,
                            FavoriteFolder* new_parent,
                            const std::string& prev_guid,
                            ChangeReason reason) override;
  virtual void ChangeFavorite(Favorite* favorite,
                              const FavoriteData& new_data,
                              ChangeReason reason) override;
  virtual void GetFavoritesByURL(
      const GURL& url,
      std::vector<const Favorite*>* favorites) override;
  int GetCount() const override;
  Favorite* GetAt(int index) const override;
  int GetIndexOf(const Favorite* favorite) const override;
  Favorite* FindByGUID(const std::string& guid) const override;
  virtual void GetFavoritesWithTitlesMatching(
      const base::string16& query,
      size_t max_count,
      std::vector<FavoriteUtils::TitleMatch>* matches) override;
  virtual void FilterFavoritePartnerContent(
      const base::hash_set<std::string>& to_stay) override;
  FavoriteFolder* GetRoot() const override;

  void Flush() override;

 protected:
  /**
   * Top level favorites folder.
   */
  std::unique_ptr<FavoriteRootFolder> root_folder_;

 private:
  virtual ~FavoriteCollectionImpl();

  /**
   * Recursively marks which partner favorites to remove.
   * All partner content favorites are marked except those on the
   * @a to_stay list. If a subfolder will become empty after removing them,
   * the folder itself is also added to the end of the @a to_remove list.
   * @param[out] to_remove list of Favorites to remove. They must be removed in
   * the order of the vector, otherwise a folder might get removed before its
   * content
   * @param folder folder from which to remove, start with root
   * @param to_stay list of guids that won't be marked
   */
  int FilterFavoritePartnerContentFromFolder(
      std::vector<Favorite*>* to_remove,
      FavoriteFolder* folder,
      const base::hash_set<std::string>& to_stay);

  /**
   * Used to order Favorites by URL.
   */
  class FavoriteURLComparator {
   public:
    bool operator()(const Favorite* n1, const Favorite* n2) const;
  };

  void Load();

  /**
   * Populates @p favorites_ordered_by_url_set_ from root.
   */
  void PopulateFavoritesByURL(const FavoriteRootFolder* root);

  /**
   * Notify listeners about collection being loaded.
   */
  void OnCollectionLoaded();

  /**
   * Notify listeners about collection going to be destroyed.
   * It is forbidden to call the collection from this method
   * and after this call.
   */
  void OnCollectionDestroyed();

  /**
   * Notify listeners about added favorite.
   */
  virtual void OnFavoriteAdded(Favorite* favorite,
                               int new_index,
                               ChangeReason reason);

  /**
   * Notify listeners about the @p favorite going to be deleted.
   */
  void OnBeforeFavoriteRemove(const Favorite* favorite,
                              const FavoriteFolder* parent,
                              int index,
                              ChangeReason reason);
  /**
   * Notify listeners that the favorite with @p favorite_guid has been deleted.
   */
  void OnAfterFavoriteRemoved(const std::string& favorite_guid,
                              ChangeReason reason);

  /**
   * Notify listeners about favorite going to be moved to new folder.
   */
  void OnFavoriteMoved(Favorite* favorite,
                       const FavoriteFolder* old_parent,
                       int old_index,
                       const FavoriteFolder* new_parent,
                       int new_index,
                       ChangeReason reason);

  /**
   * Callback invoked after data_store_ loaded data from persistent storage.
   * @p new_root is pointer to FavoriteRootFolder with loaded data.
   * From now on collection owns that pointer.
   */
  void CollectionLoaded(FavoriteRootFolder* new_root);

  /**
   * Callback invoked after saving data to persistent storage.
   */
  void CollectionSaved();

  /**
   * Update persistent storage.
   */
  void UpdatePersistentStorage();

  /**
   * Saves favorites on exit. This triggers a blocking save operations
   * that won't be cancelled by shutdown.
   */
  void UpdatePersistentStorageOnExit();

  /**
   * Prepares and sends data to storage. When force_save_on_exit is true
   * blocking save operation will be triggered.
   */
  void SaveToStorage(bool force_save_on_exit);

  /**
   * Insert the favorite to favorites_ordered_by_url_set_
   * (only if the favorite has an URL).
   */
  void AddToOrderedByURLSet(const Favorite* favorite);

  /**
   * Remove the favorite from favorites_ordered_by_url_set_
   * (only if the favorite has an URL).
   */
  void RemoveFromOrderedByURLSet(const Favorite* favorite);

  base::WeakPtrFactory<FavoriteCollectionImpl> weak_ptr_factory_;

  /**
   * List of registered observers.
   */
  base::ObserverList<FavoriteCollectionObserver> observer_list_;

  /**
   * Partner content delegate.
   */
  std::unique_ptr<FavoriteCollection::PartnerLogicDelegate> partner_logic_delegate_;

  /**
   * Favorites Index.
   */
  std::unique_ptr<FavoriteIndex> index_;

  /**
   * Data store.
   */
  std::unique_ptr<FavoriteStorage> data_store_;

  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;

  /*
   * Set of favorites ordered by URL. This is not a map to avoid copying the
   * urls.
   * WARNING: @p favorites_ordered_by_url_set_ is accessed on multiple threads.
   * As such, be sure and wrap all usage of it around
   * @p favorites_ordered_by_url_lock_.
   */
  typedef std::multiset<const Favorite*, FavoriteURLComparator>
      FavoritesOrderedByURLSet;
  FavoritesOrderedByURLSet favorites_ordered_by_url_set_;
  base::Lock favorites_ordered_by_url_lock_;
  base::Closure pending_save_;
  bool is_saving_;
  bool storage_dirty_;
};

}  // namespace opera

#endif  // COMMON_FAVORITES_FAVORITE_COLLECTION_IMPL_H_
