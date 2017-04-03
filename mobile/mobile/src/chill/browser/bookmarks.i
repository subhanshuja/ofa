// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/base_bookmark_model_observer.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/browser/opera_bookmark_utils.h"
%}

%nodefaultctor BookmarkNode;
%nodefaultdtor BookmarkNode;
class bookmarks::BookmarkNode {
 public:
  const base::string16& GetTitle() const;
  int64_t id() const;
  const GURL& url() const;
  const base::Time& date_folder_modified() const;
  bool is_folder() const;
  bool is_url() const;
  const BookmarkNode* parent() const;
  bool is_root() const;
  int child_count() const;
  bool empty() const;
  int GetTotalNodeCount() const;
  const BookmarkNode* GetChild(int index) const;
  int GetIndexOf(const BookmarkNode* node) const;
  bool HasAncestor(const BookmarkNode* node) const;
};

%rename (BookmarkModelObserver) opera::BaseBookmarkModelObserver;

namespace opera {
class BaseBookmarkModelObserver;
}

namespace bookmarks {
%nodefaultctor BookmarkModel;
%nodefaultdtor BookmarkModel;
class BookmarkModel {
 public:
  bool loaded() const;

  const BookmarkNode* root_node() const;
  const BookmarkNode* bookmark_bar_node() const;
  const BookmarkNode* other_node() const;
  const BookmarkNode* mobile_node() const;

  void AddObserver(opera::BaseBookmarkModelObserver* observer);
  void RemoveObserver(opera::BaseBookmarkModelObserver* observer);

  void Remove(const BookmarkNode* node);

  const BookmarkNode* AddURL(const BookmarkNode* parent,
                             int index,
                             const base::string16& title,
                             const GURL& url);

  const BookmarkNode* AddFolder(const BookmarkNode* parent,
                                int index,
                                const base::string16& title);

  void Move(const BookmarkNode* node,
            const BookmarkNode* new_parent,
            int index);

  void SetTitle(const BookmarkNode* node, const base::string16& title);

  void SetURL(const BookmarkNode* node, const GURL& url);

  void CommitPendingChanges();
};
}  // namespace bookmarks

namespace opera {
%feature("director", assumeoverride=1) BaseBookmarkModelObserver;
class BaseBookmarkModelObserver {
 public:
  virtual ~BaseBookmarkModelObserver();
  virtual void BookmarkModelLoaded(bookmarks::BookmarkModel* model,
                                   bool ids_reassigned) = 0;
  virtual void BookmarkModelBeingDeleted(bookmarks::BookmarkModel* model);
  virtual void BookmarkNodeAdded(bookmarks::BookmarkModel* model,
                                 const bookmarks::BookmarkNode* parent,
                                 int index) = 0;
  virtual void OnWillRemoveBookmarks(bookmarks::BookmarkModel* model,
                                     const bookmarks::BookmarkNode* parent,
                                     int old_index,
                                     const bookmarks::BookmarkNode* node);
  virtual void BookmarkNodeRemoved(bookmarks::BookmarkModel* model,
                                   const bookmarks::BookmarkNode* parent,
                                   int old_index,
                                   const bookmarks::BookmarkNode* node) = 0;
  virtual void BookmarkNodeMoved(bookmarks::BookmarkModel* model,
                                 const bookmarks::BookmarkNode* old_parent,
                                 int old_index,
                                 const bookmarks::BookmarkNode* new_parent,
                                 int new_index) = 0;
  virtual void BookmarkNodeChanged(bookmarks::BookmarkModel* model,
                                   const bookmarks::BookmarkNode* node) = 0;
  virtual void BookmarkNodeChildrenReordered(
      bookmarks::BookmarkModel* model,
      const bookmarks::BookmarkNode* node) = 0;
  virtual void BookmarkAllUserNodesRemoved(bookmarks::BookmarkModel* model) = 0;

  virtual void ExtensiveBookmarkChangesBeginning(
      bookmarks::BookmarkModel* model) {}
  virtual void ExtensiveBookmarkChangesEnded(bookmarks::BookmarkModel* model) {}
  virtual void GroupedBookmarkChangesBeginning(
      bookmarks::BookmarkModel* model) {}
  virtual void GroupedBookmarkChangesEnded(bookmarks::BookmarkModel* model) {}
};
}  // namespace opera

namespace bookmarks {
const BookmarkNode* GetBookmarkNodeByID(const BookmarkModel* model, int64_t id);
};

namespace opera {
%nodefaultctor OperaBookmarkUtils;
%nodefaultdtor OperaBookmarkUtils;
class OperaBookmarkUtils {
 public:
  static const bookmarks::BookmarkNode* GetUnsortedNode(bookmarks::BookmarkModel* model);
  static const bookmarks::BookmarkNode* GetUserRootNode(bookmarks::BookmarkModel* model);
};
}  // namespace bookmarks
