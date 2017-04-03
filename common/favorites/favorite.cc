// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/favorites/favorite.h"

#include <algorithm>
#include <iterator>

#include "base/guid.h"
#include "base/i18n/case_conversion.h"
#include "base/logging.h"
#include "base/stl_util.h"

#include "common/suggestion/query_parser.h"

namespace opera {

Favorite::Favorite(const std::string& guid, const FavoriteData& data)
    : FavoriteDataHolder(data),
      guid_(guid.empty() ? base::GenerateGUID() : guid),
      parent_(NULL) {
}

const GURL& Favorite::navigate_url() const {
  // TODO(felixe): Special handling for partner content?
  return display_url();
}

void Favorite::set_parent(FavoriteFolder* parent) {
  parent_ = parent;
}

FavoriteKeywords Favorite::GetKeywords() const {
  FavoriteKeywords keywords;

  query_parser::QueryParser parser;
  // TODO(brettw): use ICU normalization:
  // http://userguide.icu-project.org/transforms/normalization
  parser.ParseQueryWords(base::i18n::ToLower(data().title),
                         query_parser::MatchingAlgorithm::DEFAULT,
                         &keywords);

  keywords.insert(keywords.end(),
                  data().custom_keywords.begin(),
                  data().custom_keywords.end());
  return keywords;
}

bool Favorite::IsTransient() const {
  return false;
}

FavoriteSite::FavoriteSite(const std::string& guid, const FavoriteData& data)
    : Favorite(guid, data) {
}

FavoriteSite::~FavoriteSite() {
}

Favorite::Type FavoriteSite::GetType() const {
  return kFavoriteSite;
}

bool FavoriteSite::AllowURLIndexing() const {
  return true;
}

FavoriteFolder::FavoriteFolder(const std::string& guid,
                               const FavoriteData& data)
    : Favorite(guid, data) {
}

FavoriteFolder::~FavoriteFolder() {
  base::STLDeleteContainerPointers(children_.begin(), children_.end());
}

FavoriteFolder::const_iterator FavoriteFolder::begin() const {
  return children_.begin();
}

FavoriteFolder::const_iterator FavoriteFolder::end() const {
  return children_.end();
}

int FavoriteFolder::GetChildCount() const {
  return children_.size();
}

Favorite* FavoriteFolder::GetChildByIndex(int index) const {
  if (static_cast<size_t>(index) >= children_.size())
    return NULL;

  FavoriteEntries::const_iterator iter = children_.begin();
  std::advance(iter, index);

  DCHECK(iter != children_.end());
  return *iter;
}

Favorite* FavoriteFolder::FindByGUID(const std::string& guid) const {
  for (FavoriteEntries::const_iterator it = children_.begin();
       it != children_.end();
       ++it) {
    if ((*it)->guid() == guid)
      return *it;
  }
  return NULL;
}

Favorite::Type FavoriteFolder::GetType() const {
  return kFavoriteFolder;
}

bool FavoriteFolder::AllowURLIndexing() const {
  return false;
}

void FavoriteFolder::AddChildAt(Favorite* child,
                                int index) {
  DCHECK_LE(static_cast<size_t>(index), children_.size());

  if (index == static_cast<int>(children_.size())) {
    children_.push_back(child);
  } else {
    FavoriteEntries::iterator iter = children_.begin();
    std::advance(iter, index);

    DCHECK(iter != children_.end());
    children_.insert(iter, child);
  }

  child->set_parent(this);
}

void FavoriteFolder::RemoveChild(Favorite* child) {
  FavoriteEntries::iterator iter = std::find(children_.begin(),
                                             children_.end(),
                                             child);
  if (iter == children_.end())
    return;

  children_.erase(iter);
}

void FavoriteFolder::MoveChildToFolder(Favorite* child,
                                       FavoriteFolder* dest_folder,
                                       int src_position,
                                       int dest_position) {
  FavoriteEntries::iterator iter = std::find(children_.begin(),
                                             children_.end(),
                                             child);
  DCHECK(children_.end() != iter);

  if (dest_folder == this) {
    DCHECK_NE(src_position, dest_position);

    // If the favorite is in the target folder, we shift all the items between
    // the current and the new index and insert the favorite at the new
    // position. That requires different insertion index depending on if the
    // source position was on the left or right of the target position.
    if (src_position < dest_position)
      dest_position++;
  }

  dest_folder->AddChildAt(child, dest_position);

  children_.erase(iter);
}

int FavoriteFolder::GetIndexOf(const Favorite* favorite) const {
  FavoriteEntries::const_iterator iter = std::find(children_.begin(),
                                                   children_.end(),
                                                   favorite);

  if (iter != children_.end())
    return std::distance(children_.begin(), iter);

  return -1;
}

FavoriteExtension::FavoriteExtension(const std::string& guid,
                                     const FavoriteData& data,
                                     bool transient)
    : Favorite(guid, data),
      transient_(transient) {
}

FavoriteExtension::~FavoriteExtension() {
}

Favorite::Type FavoriteExtension::GetType() const {
  return kFavoriteExtension;
}

bool FavoriteExtension::AllowURLIndexing() const {
  return false;
}

bool FavoriteExtension::IsTransient() const {
  return transient_ ||
      data().extension_state == STATE_EXTENSION_INSTALLING ||
      data().extension_state == STATE_EXTENSION_INSTALL_FAILED;
}

}  // namespace opera
