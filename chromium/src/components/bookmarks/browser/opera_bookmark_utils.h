// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMPONENTS_BOOKMARKS_BROWSER_OPERA_BOOKMARK_UTILS_H_
#define COMPONENTS_BOOKMARKS_BROWSER_OPERA_BOOKMARK_UTILS_H_

#include <stdint.h>

#include <string>

class GURL;

namespace bookmarks {
class BookmarkNode;
class BookmarkModel;
}

namespace opera {

extern const char kBookmarkSpeedDialRootFolderGUIDKey[];

class OperaBookmarkUtils {
 public:
  // Returns the "unsorted" node.
  static const bookmarks::BookmarkNode* GetUnsortedNode(
      bookmarks::BookmarkModel* model);

  // Returns the "user_root" node.
  static const bookmarks::BookmarkNode* GetUserRootNode(
      bookmarks::BookmarkModel* model);

  // Returns the "speed_dial" node.
  static const bookmarks::BookmarkNode* GetSpeedDialNode(
      bookmarks::BookmarkModel* model);

  // Returns the "trash" node.
  static const bookmarks::BookmarkNode* GetTrashNode(
      bookmarks::BookmarkModel* model);

  // Will create the nodes listed above if needed and the model has loaded
  static void CreatePermanentNodesIfNeeded(bookmarks::BookmarkModel* model);

  static bool RemoveNodeToTrash(bookmarks::BookmarkModel* model,
                                int64_t id,
                                bool recursive,
                                std::string* error);

  static bool EmptyTrash(bookmarks::BookmarkModel* model, std::string* error);

  // Move all ordinary bookmarks matching the given URL to trash. This function
  // will not touch bookmarks already in trash. This is a safe alternative
  // to bookmarks::RemoveAllBookmarks() as it handles trash.
  static bool RemoveAllBookmarksToTrash(bookmarks::BookmarkModel* model,
                                        const GURL& url);

  // Remove (not trash) all folders that are other devices in the "speed_dial"
  // node.
  // Returns false if model is not loaded or local_device is not a child of
  // "speed_dial" node.
  static bool RemoveOtherDevices(bookmarks::BookmarkModel* model,
                                 const bookmarks::BookmarkNode* local_device);
};

}  // namespace opera

#endif  // COMPONENTS_BOOKMARKS_BROWSER_OPERA_BOOKMARK_UTILS_H_
