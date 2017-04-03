// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/old_bookmarks/bookmark_utils.h"
#include "base/bind.h"
#include "common/old_bookmarks/bookmark.h"
#include "url/gurl.h"

namespace {

typedef void (*CounterFunction)(int*, opera::Bookmark*, int);

void IncreaseCounter(int* counter, opera::Bookmark* bookmark,
                     int bookmark_index) {
  (*counter)++;
}

void IncreaseCounterIfFolder(int* counter, opera::Bookmark* bookmark,
                             int bookmark_index) {
  if (bookmark->GetType() == opera::Bookmark::BOOKMARK_FOLDER)
    (*counter)++;
}

int CountBookmarks(const opera::BookmarkFolder *folder, CounterFunction func) {
  int counter = 0;
  if (folder)
      opera::BookmarkUtils::ForEachBookmarkInFolder(folder,
                                                    true,
                                                    base::Bind(func, &counter));
  return counter;
}

}  // namespace

namespace opera {
namespace BookmarkUtils {

TitleMatch::TitleMatch()
    : node(NULL) {
}

TitleMatch::~TitleMatch() {}

void ForEachBookmarkInFolder(
    const opera::BookmarkFolder* folder,
    bool recurse,
    const base::Callback<void(opera::Bookmark*, int index)>& task) {
  for (int i = 0; i < folder->GetChildCount(); i++) {
    opera::Bookmark* bookmark = folder->GetChildByIndex(i);
    task.Run(bookmark, i);
    if (recurse && bookmark->GetType() == opera::Bookmark::BOOKMARK_FOLDER)
      ForEachBookmarkInFolder(
          static_cast<opera::BookmarkFolder*>(bookmark), recurse, task);
  }
}

int GetTotalCount(const BookmarkFolder *folder) {
  return CountBookmarks(folder, IncreaseCounter);
}

int GetTotalFoldersCount(const BookmarkFolder *folder) {
  return CountBookmarks(folder, IncreaseCounterIfFolder);
}

}  // namespace BookmarkUtils
}  // namespace opera
