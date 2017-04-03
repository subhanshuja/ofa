// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_COLLECTION_H_
#define MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_COLLECTION_H_

#include <string>
#include <vector>

#include "base/containers/hash_tables.h"
#include "base/memory/ref_counted.h"
#include "common/old_bookmarks/bookmark.h"

namespace opera {
class BookmarkCollectionObserver;

namespace BookmarkUtils {
struct TitleMatch;
}

class BookmarkCollection : public base::RefCounted<BookmarkCollection> {
 public:
  /**
   * The reason of any bookmark collection modification.
   * It will be passed to observers in change notifications.
   */
  enum ChangeReason {
    CHANGE_REASON_SYNC,       ///< Applying changes from the Sync server
    CHANGE_REASON_USER,       ///< Direct user action
    CHANGE_REASON_AUTOUPDATE, ///< Change from the autoupdate server
    CHANGE_REASON_OTHER
  };

  static const int kIndexPositionLast;

  /**
   * Set the observer that will receive notifications for operations on the
   * BookmarkCollection.
   */
  virtual void AddObserver(BookmarkCollectionObserver* observer) = 0;

  /**
   * Remove observer.
   */
  virtual void RemoveObserver(BookmarkCollectionObserver* observer) = 0;

  /**
   * @return true if the collection is loaded.
   */
  virtual bool IsLoaded() const = 0;

  /**
   * @return root folder if collection is loaded otherwise NULL.
   */
  virtual BookmarkFolder* GetRootFolder() const = 0;

 /**
   * Add the bookmark site to a given folder and at the given position.
   *
   * @param url The url of the new BookmarkSite.
   * @param parent The parent folder. If NULL, Bookmark will be added
                   at the top level.
   * @param index position at which the new Bookmark site should be added in
            given folder or #kIndexPositionLast to add as a last item.
   * @param guid id of bookmark extension. If an empty string is passed, a
   *        new guid will be generated.
   * @param reason The reason of addition.
   * @return pointer to a BookmarkSite or NULL in case collection
   *         was unable to add new bookmark (ie: invalid index was given or
   *         a bookmark with given non empty guid already exists).
   */
  virtual BookmarkSite* AddBookmarkSite(const std::string& url,
                                        const base::string16& name,
                                        BookmarkFolder* parent,
                                        int index,
                                        const std::string& guid,
                                        ChangeReason reason) = 0;

  /**
   * Add the bookmark folder at the given position.
   *
   * @param parent The parent folder. If NULL, folder will be added at the top
   *        level.
   * @param index The position at which the new folder should be added or
   *        #kIndexPositionLast to add at the end.
   * @param guid id of bookmark extension. If an empty string is passed, a
   *        new guid will be generated.
   * @param reason The reason of addition.
   * @return pointer to a BookmarkFolder or NULL in case collection
   *         was unable to add new bookmark (ie: invalid index was given or
   *         a bookmark with given non empty guid already exists).
   */
  virtual BookmarkFolder* AddBookmarkFolder(const base::string16& name,
                                            BookmarkFolder* parent,
                                            int index,
                                            const std::string& guid,
                                            ChangeReason reason) = 0;

  /**
   * Remove the bookmark specified as parameter.
   *
   * @param entry The bookmark to be removed.
   * @param reason The reason of removal.
   * @return true if the bookmark was successfully removed.
   */
  virtual void RemoveBookmark(Bookmark* bookmark, ChangeReason reason) = 0;

  /**
   * Move the Bookmark to a new position.
   *
   * @param entry The bookmark that is moved.
   * @param new_parent The parent the bookmark should be moved to, or NULL
   *        if the bookmark is to be moved to the root folder.
   * @param new_index The index at which the bookmark is to be inserted
   *        or #kIndexPositionLast to insert at the end.
   * @param reason The reason of moving.
   * @return true if Bookmark was moved, false otherwise.
   */
  virtual bool MoveBookmark(Bookmark* bookmark,
                            BookmarkFolder* new_parent,
                            int new_index,
                            ChangeReason reason) = 0;

  /**
   * Rename bookmark.
   *
   * @param bookmark The bookmark to rename.
   * @param new_name The new name.
   * @param reason The reason of renaming.
   */
  virtual void RenameBookmark(Bookmark* bookmark,
                              const base::string16& new_name,
                              ChangeReason reason) = 0;

  /**
   * Change URL of the bookmark.
   *
   * @param bookmark The bookmark to change URL of.
   * @param new_url The URL to be set.
   * @param reason The reason of changing url.
   */
  virtual void ChangeBookmarkURL(Bookmark* bookmark,
                                 const std::string& new_url,
                                 ChangeReason reason) = 0;

  /**
   * Notify the collection that the bookmark has changed state.
   *
   * @param bookmark The bookmark which has changed.
   * @param reason The reason for the change.
   */
  virtual void NotifyBookmarkChange(Bookmark* bookmark,
                                    ChangeReason reason) = 0;


  /**
   * Returns the set of bookmarks having a given url.
   *
   * @param url The url to search for
   * @param bookmarks A vector of bookmarks to be filled with the
   *                  bookmarks having the requested URL. The
   *                  BookmarkCollection retains ownership of the
   *                  bookmarks
   */
  virtual void GetBookmarksByURL(const std::string& url,
                                 std::vector<const Bookmark*>* bookmarks) = 0;

  /**
   * Get the number of bookmarks present at root level.
   *
   * @return The count of bookmarks at the root level.
   */
  virtual int GetCount() const = 0;

  /**
   * @return the Bookmark at @p index index.
   */
  virtual Bookmark* GetAt(int index) const = 0;

  /**
   * @param guid The GUID of the Bookmark that is to be found.
   *
   * @return The Bookmark matching given @p guid or NULL.
   */
  virtual Bookmark* FindByGUID(const std::string& guid) const = 0;

  /**
   * Returns up to @p max_count of bookmarks containing the text @p query.
   */
  virtual void GetBookmarksWithTitlesMatching(const base::string16& query,
      size_t max_count, std::vector<BookmarkUtils::TitleMatch>* matches) = 0;

  /**
   * Store any unsaved data to disk.
   */
  virtual void Flush() = 0;

 protected:
  friend class base::RefCounted<BookmarkCollection>;
  virtual ~BookmarkCollection() {}
};

}  // namespace opera

#endif  // MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_COLLECTION_H_
