// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_COLLECTION_IMPL_H_
#define MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_COLLECTION_IMPL_H_

#include <set>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"

#include "common/old_bookmarks/bookmark.h"
#include "common/old_bookmarks/bookmark_collection.h"
#include "common/old_bookmarks/bookmark_index.h"
#include "common/old_bookmarks/bookmark_storage.h"

namespace opera {

/**
 * Bookmark Root Folder.
 *
 * Implements local bookmark storage reusing existing BookmarkFolder
 * implementation.
 */
class BookmarkRootFolder : public BookmarkFolder {
 public:
  BookmarkRootFolder();

  /**
   * Add @p child to @p parent at @p index.
   *
   * @param index the index where the child is added.  If it's
   *    BookmarkCollection::kIndexPositionLast, it receives the actual index
   *    where the child is added.
   */
  void AddChildAt(BookmarkFolder* parent, Bookmark* child, int* index);

  /**
   * Remove @p bookmark.
   */
  void Remove(Bookmark* bookmark);

  /**
   * Move @p bookmark to @p move_to_folder at @p move_to_index new position.
   */
  void Move(Bookmark* bookmark,
            BookmarkFolder* move_to_folder,
            int move_from_index,
            int move_to_index);

  /**
   * Get top level child count.
   */
  int GetChildCount() const;

  /**
   * Get child at @p index.
   */
  Bookmark* GetChildByIndex(int index) const;

  /**
   * Recreate bookmark from given data. No notification is being sent
   * when recreating bookmarks. This is used to recreate local bookmark
   * storage from data kept in persistent storage.
   */
  bool RecreateBookmark(const std::string& guid,
                        const base::string16& name,
                        const std::string& url,
                        int type,
                        const std::string& parent_guid);

  bool IsRootFolder() const override;
};

class BookmarkCollectionImpl : public BookmarkCollection {
 public:
  BookmarkCollectionImpl(
      BookmarkStorage* storage,
      scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner);

  // TODO(felixe): Perhaps this secure shutdown thingie is something good? ;-)
  //               We should probably look into destroying the object through
  //               it
  virtual void Shutdown();

  // BookmarkCollection implementation:
  void AddObserver(BookmarkCollectionObserver* observer) override;
  void RemoveObserver(BookmarkCollectionObserver* observer) override;
  bool IsLoaded() const override;
  BookmarkFolder* GetRootFolder() const override;
  virtual BookmarkSite* AddBookmarkSite(const std::string& url,
                                        const base::string16& name,
                                        BookmarkFolder* parent,
                                        int index,
                                        const std::string& guid,
                                        ChangeReason reason) override;
  virtual BookmarkFolder* AddBookmarkFolder(const base::string16& name,
                                            BookmarkFolder* parent,
                                            int index,
                                            const std::string& guid,
                                            ChangeReason reason) override;
  void RemoveBookmark(Bookmark* bookmark, ChangeReason reason) override;
  virtual bool MoveBookmark(Bookmark* bookmark,
                            BookmarkFolder* dest_folder,
                            int new_index,
                            ChangeReason reason) override;
  virtual void RenameBookmark(Bookmark* bookmark,
                              const base::string16& new_name,
                              ChangeReason reason) override;
  virtual void ChangeBookmarkURL(Bookmark* bookmark,
                                 const std::string& new_url,
                                 ChangeReason reason) override;
  virtual void NotifyBookmarkChange(Bookmark* bookmark,
                                    ChangeReason reason) override;
  virtual void GetBookmarksByURL(
      const std::string& url,
      std::vector<const Bookmark*>* bookmarks) override;
  int GetCount() const override;
  Bookmark* GetAt(int index) const override;
  Bookmark* FindByGUID(const std::string& guid) const override;
  virtual void GetBookmarksWithTitlesMatching(
      const base::string16& query,
      size_t max_count,
      std::vector<BookmarkUtils::TitleMatch>* matches) override;

  void Flush() override;

 private:
  virtual ~BookmarkCollectionImpl();

  void Load();

  /**
   * Used to order Bookmarks by URL.
   */
  class BookmarkURLComparator {
   public:
    bool operator()(const Bookmark* n1, const Bookmark* n2) const;
  };

  /**
   * Populates @p bookmarks_ordered_by_url_set_ from root.
   */
  void PopulateBookmarksByURL(const BookmarkRootFolder* root);

  void PopulateBookmarksByURLForFolder(const BookmarkFolder* folder);

  /**
   * Calls OnRecreated on each Bookmark.
   */
  void CallBookmarkOnRecreated(BookmarkFolder* root);

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
   * Notify listeners about added bookmark.
   */
  virtual void OnBookmarkAdded(const Bookmark* bookmark,
                               int new_index,
                               ChangeReason reason);

  /**
   * Notify listeners about bookmark going to be deleted.
   */
  void OnBookmarkRemoved(const Bookmark* bookmark,
                         const BookmarkFolder* parent,
                         int index,
                         ChangeReason reason);
  /**
   * Notify listeners about bookmark going to be moved to new folder.
   */
  void OnBookmarkMoved(const Bookmark* bookmark,
                       const BookmarkFolder* old_parent,
                       int old_index,
                       const BookmarkFolder* new_parent,
                       int new_index,
                       ChangeReason reason);

  /**
   * Notify listeners about bookmark being changed.
   */
  virtual void OnBookmarkChanged(const Bookmark* bookmark, ChangeReason reason);

  /**
   * Callback invoked after data_store_ loaded data from persistent storage.
   * @p new_root is pointer to BookmarkRootFolder with loaded data.
   * From now on collection owns that pointer.
   */
  void CollectionLoaded(BookmarkRootFolder* new_root);

  /**
   * Callback invoked after saving data to persistent storage.
   */
  void CollectionSaved();

  /**
   * Update persistent storage.
   */
  void UpdatePersistentStorage();

  /**
   * Saves bookmarks on exit. This triggers a blocking save operations
   * that won't be cancelled by shutdown.
   */
  void UpdatePersistentStorageOnExit();

  /**
   * Prepares and sends data to storage. When force_save_on_exit is true
   * a save is always triggered.
   */
  void SaveToStorage(bool force_save_on_exit);

  /**
   * Insert the bookmark to bookmarks_ordered_by_url_set_
   * (only if the bookmark has an URL).
   */
  void AddToOrderedByURLSet(const Bookmark* bookmark);

  /**
   * Remove the bookmark from bookmarks_ordered_by_url_set_
   * (only if the bookmark has an URL).
   */
  void RemoveFromOrderedByURLSet(const Bookmark* bookmark);

  base::WeakPtrFactory<BookmarkCollectionImpl> weak_ptr_factory_;

  /**
   * Top level bookmarks folder.
   */
  std::unique_ptr<BookmarkRootFolder> root_folder_;

  /**
   * List of registered observers.
   */
  base::ObserverList<BookmarkCollectionObserver> observer_list_;

#ifndef BOOKMARKS_NO_INDEX
  /**
   * Favorites Index.
   */
  std::unique_ptr<BookmarkIndex> index_;
#endif

  /**
   * Data store.
   */
  std::unique_ptr<BookmarkStorage> data_store_;

  /**
   * Task runner for the thread on which UI operations should be
   * performed.
   */
  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;

  /*
   * Set of bookmarks ordered by URL. This is not a map to avoid copying the
   * urls.
   * WARNING: @p bookmarks_ordered_by_url_set_ is accessed on multiple threads.
   * As such, be sure and wrap all usage of it around
   * @p bookmarks_ordered_by_url_lock_.
   */
  typedef std::multiset<const Bookmark*, BookmarkURLComparator>
      BookmarksOrderedByURLSet;
  BookmarksOrderedByURLSet bookmarks_ordered_by_url_set_;
  base::Lock bookmarks_ordered_by_url_lock_;
  base::Closure pending_save_;
  bool is_saving_;
  bool storage_dirty_;
};

}  // namespace opera

#endif  // MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_COLLECTION_IMPL_H_
