// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_COLLECTION_LOAD_OBSERVER_H_
#define MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_COLLECTION_LOAD_OBSERVER_H_

#include "common/old_bookmarks/bookmark_collection_observer.h"

namespace opera {

/**
 * A helper class for BookmarkCollectionObservers that are only interested
 * in the BookmarkCollectionLoaded notification.
 *
 * Provides empty implementations for all methods of BookmarkCollectionObserver
 * except BookmarkCollectionLoaded.
 */
class BookmarkCollectionLoadObserver : public BookmarkCollectionObserver {
 public:
  // BookmarkCollectionObserver:
  void BookmarkCollectionDeleted() override {}
  virtual void BookmarkAdded(
      const Bookmark* bookmark,
      int new_index,
      BookmarkCollection::ChangeReason reason) override {}
  virtual void BookmarkRemoved(
      const Bookmark* bookmark,
      const BookmarkFolder* parent,
      int index,
      BookmarkCollection::ChangeReason reason) override {}
  virtual void BookmarkMoved(
      const Bookmark* bookmark,
      const BookmarkFolder* old_parent,
      int old_index,
      const BookmarkFolder* new_parent,
      int new_index,
      BookmarkCollection::ChangeReason reason) override {}
  virtual void BookmarkChanged(
      const Bookmark* bookmark,
      BookmarkCollection::ChangeReason reason) override {}

 protected:
  virtual ~BookmarkCollectionLoadObserver() {}
};

}  // namespace opera

#endif  // MOBILE_COMMON_OLD_BOOKMARKS_BOOKMARK_COLLECTION_LOADED_OBSERVER_H_
