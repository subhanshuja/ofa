// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_BASE_BOOKMARK_MODEL_OBSERVER_H_
#define CHILL_BROWSER_BASE_BOOKMARK_MODEL_OBSERVER_H_

#include <set>

#include "components/bookmarks/browser/bookmark_model_observer.h"

namespace opera {

class BaseBookmarkModelObserver : public bookmarks::BookmarkModelObserver {
 public:
  virtual ~BaseBookmarkModelObserver() {}

  virtual void BookmarkNodeRemoved(bookmarks::BookmarkModel* model,
                                   const bookmarks::BookmarkNode* parent,
                                   int old_index,
                                   const bookmarks::BookmarkNode* node) = 0;

  void BookmarkNodeRemoved(
      bookmarks::BookmarkModel* model,
      const bookmarks::BookmarkNode* parent,
      int old_index,
      const bookmarks::BookmarkNode* node,
      const std::set<GURL>& removed_urls) override {
    BookmarkNodeRemoved(model, parent, old_index, node);
  }

  void BookmarkNodeFaviconChanged(bookmarks::BookmarkModel* model,
                                  const bookmarks::BookmarkNode* node)
                                          override {}

  virtual void BookmarkAllUserNodesRemoved(bookmarks::BookmarkModel* model) = 0;

  void BookmarkAllUserNodesRemoved(
      bookmarks::BookmarkModel* model,
      const std::set<GURL>& removed_urls) override {
    BookmarkAllUserNodesRemoved(model);
  }
};

}  // namespace opera

#endif  // CHILL_BROWSER_BASE_BOOKMARK_MODEL_OBSERVER_H_
