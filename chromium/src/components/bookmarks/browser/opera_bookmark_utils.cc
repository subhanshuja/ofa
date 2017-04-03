// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "components/bookmarks/browser/opera_bookmark_utils.h"

#include "components/bookmarks/browser/bookmark_client.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "url/gurl.h"

using bookmarks::BookmarkModel;

namespace {

// Custom node names
// NOTE: Changes to custom nodes (adding, removing, renaming) must be
// implemented on the Sync server as well. See bookmark_model_associator.cc.
const char kBookmarkNodeUnsorted[] = "unsorted";
const char kBookmarkNodeUserRoot[] = "userRoot";
const char kBookmarkNodeSpeedDial[] = "speedDial";
const char kBookmarkNodeTrash[] = "trash";

}  // namespace

namespace {
const char kNoNodeError[] = "Can't find bookmark for id.";
const char kModifySpecialError[] = "Can't modify the root bookmark folders.";
const char kFolderNotEmptyError[] =
    "Can't remove non-empty folder (use recursive to force).";
}

namespace opera {

const char kBookmarkSpeedDialRootFolderGUIDKey[] =
    "speed_dial_root_folder_guid";

// static
const bookmarks::BookmarkNode* OperaBookmarkUtils::GetUnsortedNode(
    BookmarkModel* model) {
  if (model->loaded())
    return model->CustomPermanentNode(kBookmarkNodeUnsorted);
  return nullptr;
}

// static
const bookmarks::BookmarkNode* OperaBookmarkUtils::GetUserRootNode(
    BookmarkModel* model) {
  if (model->loaded())
    return model->CustomPermanentNode(kBookmarkNodeUserRoot);
  return nullptr;
}

// static
const bookmarks::BookmarkNode* OperaBookmarkUtils::GetSpeedDialNode(
    BookmarkModel* model) {
  if (model->loaded())
    return model->CustomPermanentNode(kBookmarkNodeSpeedDial);
  return nullptr;
}

// static
const bookmarks::BookmarkNode* OperaBookmarkUtils::GetTrashNode(
    BookmarkModel* model) {
  if (model->loaded())
    return model->CustomPermanentNode(kBookmarkNodeTrash);

  return nullptr;
}

// static
void OperaBookmarkUtils::CreatePermanentNodesIfNeeded(BookmarkModel* model) {
  GetUnsortedNode(model);
  GetUserRootNode(model);
  GetSpeedDialNode(model);
  GetTrashNode(model);
}

// static
bool OperaBookmarkUtils::RemoveNodeToTrash(BookmarkModel* model,
                                           int64_t id,
                                           bool recursive,
                                           std::string* error) {
  const bookmarks::BookmarkNode* node =
      bookmarks::GetBookmarkNodeByID(model, id);
  if (!node) {
    *error = kNoNodeError;
    return false;
  }
  bookmarks::BookmarkClient* client = model->client();
  if (model->is_permanent_node(node) || !client->CanBeEditedByUser(node)) {
    *error = kModifySpecialError;
    return false;
  }
  if (node->is_folder() && !node->empty() && !recursive) {
    *error = kFolderNotEmptyError;
    return false;
  }
  const bookmarks::BookmarkNode* trash_node =
      OperaBookmarkUtils::GetTrashNode(model);
  const bookmarks::BookmarkNode* parent = node->parent();
  if (trash_node == parent) {
    // Remove from trash is allowed
    model->Remove(node);
  } else {
    model->Move(node, trash_node, 0);
  }
  return true;
}

// static
bool OperaBookmarkUtils::EmptyTrash(BookmarkModel* model, std::string* error) {
  const bookmarks::BookmarkNode* trash_node =
      OperaBookmarkUtils::GetTrashNode(model);
  if (!trash_node) {
    *error = kNoNodeError;
    return false;
  }
  int count = trash_node->child_count();
  while (count > 0) {
    model->Remove(trash_node->GetChild(0));
    count--;
  }
  return true;
}

// static
bool OperaBookmarkUtils::RemoveAllBookmarksToTrash(BookmarkModel* model,
                                                   const GURL& url) {
  const bookmarks::BookmarkNode* trash_node =
      OperaBookmarkUtils::GetTrashNode(model);
  if (!trash_node)
    return false;

  std::vector<const bookmarks::BookmarkNode*> bookmarks;
  model->GetNodesByURL(url, &bookmarks);
  std::string error;
  bookmarks::BookmarkClient* client = model->client();

  // Remove all the matching user bookmarks except trash items.
  for (const bookmarks::BookmarkNode* node : bookmarks) {
    if (client->CanBeEditedByUser(node) && !node->HasAncestor(trash_node)) {
      if (!RemoveNodeToTrash(model, node->id(), true, &error))
        return false;
    }
  }
  return true;
}

// static
bool OperaBookmarkUtils::RemoveOtherDevices(
    bookmarks::BookmarkModel* model,
    const bookmarks::BookmarkNode* local_device) {
  const bookmarks::BookmarkNode* root = GetSpeedDialNode(model);
  if (!root)
    return false;
  if (!local_device || local_device->parent() != root)
    return false;

  int i = root->child_count();
  while (i--) {
    auto child = root->GetChild(i);
    if (child != local_device) {
      model->Remove(child);
    }
  }
  return true;
}


}  // namespace opera
