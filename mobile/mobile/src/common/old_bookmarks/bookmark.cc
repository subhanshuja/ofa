// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/old_bookmarks/bookmark.h"

#include <algorithm>
#include <iterator>

#include "base/guid.h"
#include "base/i18n/case_conversion.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"

#include "common/suggestion/query_parser.h"

namespace opera {

Bookmark::Bookmark(const base::string16& name,
                   const std::string& url,
                   const std::string& guid)
    : title_(name),
      url_(url),
      guid_(guid.empty() ? base::GenerateGUID() : guid),
      parent_(NULL) {
}

Bookmark::~Bookmark() {
}

void Bookmark::set_parent(BookmarkFolder* parent) {
  parent_ = parent;
}

Bookmark::Keywords Bookmark::GetKeyWords() const {
  std::vector<base::string16> terms;
  if (title_.empty())
    return terms;

  query_parser::QueryParser parser;
  // TODO(brettw): use ICU normalization:
  // http://userguide.icu-project.org/transforms/normalization
  parser.ParseQueryWords(base::i18n::ToLower(title_),
                         query_parser::MatchingAlgorithm::DEFAULT,
                         &terms);
  return terms;
}

void Bookmark::SetUrl(const std::string& url, bool by_user_action) {
  url_ = url;
}

void Bookmark::SetTitle(const base::string16& title, bool by_user_action) {
  title_ = title;
}

BookmarkSite::BookmarkSite(const base::string16& name,
                           const std::string& url,
                           const std::string& guid)
    : Bookmark(name, url, guid) {
}

Bookmark::Type BookmarkSite::GetType() const {
  return BOOKMARK_SITE;
}

bool BookmarkSite::AllowURLIndexing() const {
  return true;
}

BookmarkFolder::BookmarkFolder(const base::string16& name,
                               const std::string& guid)
    : Bookmark(name, "", guid) {
}

BookmarkFolder::~BookmarkFolder() {
  base::STLDeleteContainerPointers(children_.begin(), children_.end());
}

int BookmarkFolder::GetChildCount() const {
  return children_.size();
}

Bookmark* BookmarkFolder::GetChildByIndex(int index) const {
  if (static_cast<size_t>(index) >= children_.size())
    return NULL;

  BookmarkEntries::const_iterator iter = children_.begin();
  std::advance(iter, index);

  DCHECK(iter != children_.end());
  return *iter;
}

Bookmark* BookmarkFolder::FindByGUID(const std::string& guid) const {
  for (BookmarkEntries::const_iterator iter = children_.begin();
      iter != children_.end();
      ++iter) {
    if ((*iter)->guid() == guid)
      return *iter;

    if ((*iter)->GetType() == Bookmark::BOOKMARK_FOLDER) {
      Bookmark* found_bookmark =
          static_cast<BookmarkFolder*>(*iter)->FindByGUID(guid);
      if (found_bookmark)
        return found_bookmark;
    }
  }
  return NULL;
}

Bookmark::Type BookmarkFolder::GetType() const {
  return BOOKMARK_FOLDER;
}

bool BookmarkFolder::AllowURLIndexing() const {
  return false;
}

void BookmarkFolder::AddChildAt(Bookmark* child, int index) {
  DCHECK_LE(static_cast<size_t>(index), children_.size());

  if (index == static_cast<int>(children_.size())) {
    children_.push_back(child);
  } else {
    BookmarkEntries::iterator iter = children_.begin();
    std::advance(iter, index);

    DCHECK(iter != children_.end());
    children_.insert(iter, child);
  }

  child->set_parent(this);
}

void BookmarkFolder::RemoveChild(Bookmark* child) {
  BookmarkEntries::iterator iter =
      std::find(children_.begin(), children_.end(), child);

  if (iter == children_.end())
    return;

  delete *iter;
  children_.erase(iter);
}

void BookmarkFolder::MoveChildToFolder(Bookmark* child,
                                       BookmarkFolder* dest_folder,
                                       int src_position,
                                       int dest_position) {
  BookmarkEntries::iterator iter = std::find(children_.begin(),
                                             children_.end(),
                                             child);
  DCHECK(children_.end() != iter);

  if (dest_folder == this) {
    DCHECK_NE(src_position, dest_position);

    // If the bookmark is in the target folder, we shift all the items between
    // the current and the new index and insert the bookmark at the new
    // position. That requires different insertion index depending on if the
    // source position was on the left or right of the target position.
    if (src_position < dest_position)
      dest_position++;
  }

  dest_folder->AddChildAt(child, dest_position);

  children_.erase(iter);
}

bool BookmarkFolder::IsRootFolder() const {
  return false;
}

int BookmarkFolder::GetIndexOf(const Bookmark* bookmark) const {
  BookmarkEntries::const_iterator iter =
      std::find(children_.begin(), children_.end(), bookmark);

  if (iter != children_.end())
    return std::distance(children_.begin(), iter);

  return -1;
}


}  // namespace opera
