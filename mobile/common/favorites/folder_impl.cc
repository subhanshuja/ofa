// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/favorites/favorites_impl.h"
#include "mobile/common/favorites/folder_impl.h"

#include <algorithm>

namespace mobile {

FolderImpl::FolderImpl(FavoritesImpl* favorites,
                       int64_t parent,
                       int64_t id,
                       const base::string16& title,
                       const std::string& guid,
                       const std::string& thumbnail_path)
    : Folder(parent, id),
      favorites_(favorites),
      title_(title),
      guid_(guid),
      thumbnail_path_(thumbnail_path),
      check_(0) {
}

FolderImpl::~FolderImpl() {
}

bool FolderImpl::IsPartnerContent() const {
  return false;
}

bool FolderImpl::CanChangeTitle() const {
  return true;
}

bool FolderImpl::CanChangeParent() const {
  return true;
}

Folder* FolderImpl::GetParent() const {
  return static_cast<Folder*>(favorites_->GetFavorite(parent()));
}

const base::string16& FolderImpl::title() const {
  return title_;
}

void FolderImpl::SetTitle(const base::string16& title) {
  if (title_ == title)
    return;
  title_ = title;
  favorites_->SignalChanged(id(), parent(), FavoritesObserver::TITLE_CHANGED);
}

const std::string& FolderImpl::guid() const {
  return guid_;
}

const std::string& FolderImpl::thumbnail_path() const {
  return thumbnail_path_;
}

void FolderImpl::SetThumbnailPath(const std::string& path) {
  if (thumbnail_path_.empty() && path.empty())
    return;
  thumbnail_path_ = path;
  favorites_->SignalChanged(
      id(), parent(), FavoritesObserver::THUMBNAIL_CHANGED);
}

bool FolderImpl::CanTakeMoreChildren() const {
  return true;
}

int FolderImpl::Size() const {
  return children_.size();
}

int FolderImpl::TotalSize() {
  int count = 0;
  for (children_t::iterator i(children_.begin()); i != children_.end(); ++i) {
    Favorite *f = favorites_->GetFavorite(*i);
    if (f->IsFolder())
      count += ((Folder*)f)->TotalSize();

    ++count;
  }

  return count;
}

int FolderImpl::FoldersCount() {
  int count = 0;
  for (children_t::iterator i(children_.begin()); i != children_.end(); ++i) {
    Favorite *f = favorites_->GetFavorite(*i);
    if (f->IsFolder())
      ++count;
  }

  return count;
}

const std::vector<int64_t>& FolderImpl::Children() const {
  return children_;
}

Favorite* FolderImpl::Child(int index) {
  DCHECK_GE(index, 0);
  if (index >= static_cast<int>(children_.size()))
    return NULL;
  return favorites_->GetFavorite(children_[index]);
}

void FolderImpl::Add(int pos, Favorite* favorite) {
  children_t::iterator i;
  DCHECK_GE(pos, 0);
  pos = std::min(pos, static_cast<int>(children_.size()));
  i = children_.begin() + pos;
  if (favorite->parent() != id()) {
    // The favorite is moved from another parent to this one
    FolderImpl* old =
        static_cast<FolderImpl*>(favorites_->GetFavorite(favorite->parent()));
    int old_pos = old->IndexOf(favorite);
    old->children_.erase(old->children_.begin() + old_pos);
    pos = i - children_.begin();
    children_.insert(i, favorite->id());
    changeParentToMe(favorite);
    favorites_->SignalMoved(favorite->id(), old->id(), old_pos, id(), pos);
    old->Check();
  } else {
    children_t::iterator j(
        std::find(children_.begin(), children_.end(), favorite->id()));
    if (j != children_.end()) {
      // A move inside the folder
      const int old_pos = j - children_.begin();
      if (old_pos == pos)
        return;
      children_.erase(j);
      pos = std::min(pos, static_cast<int>(children_.size()));
      i = children_.begin() + pos;
      children_.insert(i, favorite->id());
      favorites_->SignalMoved(favorite->id(), id(), old_pos, id(), pos);
    } else {
      // Favorite is added to this folder for the first time
      children_.insert(i, favorite->id());
      favorites_->SignalAdded(favorite->id(), id(), pos);
    }
  }
}

void FolderImpl::Move(int old_position, int new_position) {
  DCHECK_GE(old_position, 0);
  DCHECK_GE(new_position, 0);
  DCHECK_LT(old_position, static_cast<int>(children_.size()));
  new_position = std::min(new_position, static_cast<int>(children_.size()));
  if (old_position == new_position)
    return;
  children_t::iterator i(children_.begin() + old_position);
  int64_t id = *i;
  children_.erase(i);
  new_position = std::min(new_position, static_cast<int>(children_.size()));
  i = children_.begin() + new_position;
  children_.insert(i, id);
  favorites_->SignalMoved(
      id, this->id(), old_position, this->id(), new_position);
}

void FolderImpl::ForgetAbout(Favorite* favorite) {
  children_t::iterator i(std::find(children_.begin(),
                                   children_.end(),
                                   favorite->id()));
  DCHECK(i != children_.end());
  children_.erase(i);
}

int FolderImpl::IndexOf(int64_t id) {
  children_t::iterator i(std::find(children_.begin(), children_.end(), id));
  return i != children_.end() ? i - children_.begin() : -1;
}

int FolderImpl::IndexOf(const std::string& guid) {
  for (children_t::iterator i(children_.begin()); i != children_.end(); ++i) {
    if (favorites_->GetFavorite(*i)->guid() == guid) {
      return i - children_.begin();
    }
  }
  return -1;
}

int32_t FolderImpl::PartnerGroup() const {
  return 0;
}

void FolderImpl::SetPartnerGroup(int32_t group) {
  NOTREACHED();
}

void FolderImpl::Activated() {}

// If you change the implementation of Check(), please double-check (no pun
// intended) that FavoritesImpl::ForEachFolder works with it
bool FolderImpl::Check() {
  if (check_ > 0)
    return false;
  if (children_.size() >= 2)
    return false;
  FolderImpl* parent =
      static_cast<FolderImpl*>(favorites_->GetFavorite(this->parent()));
  if (children_.empty()) {
    Remove();
    return true;
  } else {
    int index = parent->IndexOf(this);
    DCHECK_GE(index, 0);
    // This will cause Check() to be called again, this time with
    // an empty children_ array
    parent->Add(index, favorites_->GetFavorite(children_.front()));
    return false;
  }
}

void FolderImpl::AddAll(Folder* folder) {
  FolderImpl* f = static_cast<FolderImpl*>(folder);
  f->DisableCheck();
  while (f->Size() > 0) {
    Add(f->Child(0));
  }
  f->RestoreCheck();
}

void FolderImpl::DisableCheck() {
  check_++;
}

void FolderImpl::RestoreCheck() {
  DCHECK_GT(check_, 0);
  if (--check_ == 0)
    Check();
}

void FolderImpl::RestoreCheckWithoutAction() {
  DCHECK_GT(check_, 0);
  --check_;
}

}  // namespace mobile
