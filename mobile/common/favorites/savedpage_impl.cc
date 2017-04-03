// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/favorites/savedpage_impl.h"

#include "mobile/common/favorites/favorites_impl.h"

namespace mobile {

SavedPageImpl::SavedPageImpl(FavoritesImpl* favorites,
                             int64_t parent,
                             int64_t id,
                             const base::string16& title,
                             const GURL& url,
                             const std::string& guid,
                             const std::string& file,
                             const std::string& thumbnail)
    : SavedPage(parent, id),
      favorites_(favorites),
      title_(title),
      url_(url),
      thumbnail_path_(thumbnail),
      guid_(guid),
      file_(file) {}

SavedPageImpl::~SavedPageImpl() {
  if (!file_.empty()) {
    favorites_->DeleteSavedPageFile(file_);
  }
}

bool SavedPageImpl::CanChangeTitle() const { return true; }

Favorite::ThumbnailMode SavedPageImpl::GetThumbnailMode() const {
  base::FilePath file_path(file_);
  if (file_path.MatchesExtension(".obml16")) {
    // Migrated saved pages should always use fallback thumbnail
    return Favorite::ThumbnailMode::FALLBACK;
  }
  return Favorite::GetThumbnailMode();
}

Folder* SavedPageImpl::GetParent() const {
  return static_cast<Folder*>(favorites_->GetFavorite(parent()));
}

void SavedPageImpl::Remove() { favorites_->RemoveNonNodeFavorite(this); }

const std::string& SavedPageImpl::guid() const { return guid_; }

const base::string16& SavedPageImpl::title() const { return title_; }

void SavedPageImpl::SetTitle(const base::string16& title) {
  if (title_ == title)
    return;
  title_ = title;
  favorites_->SignalChanged(id(), parent(), FavoritesObserver::TITLE_CHANGED);
  static_cast<SavedPagesFolder*>(GetParent())->ScheduleWrite();
}

const GURL& SavedPageImpl::url() const { return url_; }

void SavedPageImpl::SetURL(const GURL& url) {
  if (url_ == url)
    return;
  url_ = url;
  favorites_->SignalChanged(id(), parent(), FavoritesObserver::URL_CHANGED);
}

const std::string& SavedPageImpl::thumbnail_path() const {
  return thumbnail_path_;
}

void SavedPageImpl::SetThumbnailPath(const std::string& path) {
  if (thumbnail_path_.empty() && path.empty())
    return;
  thumbnail_path_ = path;
  favorites_->SignalChanged(
      id(), parent(), FavoritesObserver::THUMBNAIL_CHANGED);
}

const std::string& SavedPageImpl::file() const { return file_; }

void SavedPageImpl::SetFile(const std::string& file, bool force) {
  if (!force && file_ == file)
      return;
  file_ = file;
  favorites_->SignalChanged(id(), parent(), FavoritesObserver::FILE_CHANGED);
  static_cast<SavedPagesFolder*>(GetParent())->ScheduleWrite();
}

void SavedPageImpl::SetFile(const std::string& file) {
  SetFile(file, false);
}

void SavedPageImpl::Activated() {}

}  // namespace mobile
