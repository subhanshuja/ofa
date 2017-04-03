// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_COLLECTION_OBSERVER_H_
#define MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_COLLECTION_OBSERVER_H_

#include "common/old_bookmarks/bookmark_collection.h"

namespace opera {

/**
 * A BookmarkCollectionObserver is notified of changes to the
 * BookmarkCollection.
 */
class BookmarkCollectionObserver {
 public:
  virtual ~BookmarkCollectionObserver() {}

  /**
   * Called when the Collection has finished loading.
   */
  virtual void BookmarkCollectionLoaded() = 0;

  /**
   * Called when the Collection is deleted.
   * It is forbidden to call the collection from this method
   * and after this call.
   */
  virtual void BookmarkCollectionDeleted() = 0;

  /**
   * Invoked when a Bookmark has been added.
   * @param bookmark The bookmark that was changed.
   * @param new_index The new index of the bookmark.
   * @param reason The reason of addition that was provided by the source
   *        of modification.
   */
  virtual void BookmarkAdded(const Bookmark* bookmark,
                             int new_index,
                             BookmarkCollection::ChangeReason reason) = 0;

  /**
   * Invoked when a Bookmark has been removed.
   *
   * @param bookmark The Bookmark that is removed.
   * @param parent The parent of the removed bookmark.
   * @param index The index of the removed bookmark.
   * @param reason The reason of removal that was provided by the source
   *        of modification.
   */
  virtual void BookmarkRemoved(const Bookmark* bookmark,
                               const BookmarkFolder* parent,
                               int index,
                               BookmarkCollection::ChangeReason reason) = 0;

  /**
   * Invoked when a Bookmark has moved.
   * Note that this might mean its old parent is also removed.
   *
   * @param bookmark The bookmark that was moved.
   * @param new_parent The new parent of the bookmark, or NULL if it was moved
   *                   to the root level.
   * @param new_index The index of the bookmark after the move.
   * @param reason The reason of move that was provided by the source
   *        of modification.
   */
  virtual void BookmarkMoved(const Bookmark* bookmark,
                             const BookmarkFolder* old_parent,
                             int old_index,
                             const BookmarkFolder* new_parent,
                             int new_index,
                             BookmarkCollection::ChangeReason reason) = 0;

  /**
   * Notify collection about change in Bookmark private data.
   * @param bookmark The bookmark that was changed.
   * @param reason The reason of change that was provided by the source
   *        of modification.
   */
  virtual void BookmarkChanged(const Bookmark* bookmark,
                               BookmarkCollection::ChangeReason reason) = 0;
};

}  // namespace opera

#endif  // MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_COLLECTION_OBSERVER_H_
