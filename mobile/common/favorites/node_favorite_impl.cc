// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/favorites/node_favorite_impl.h"

#include "mobile/common/favorites/favorites_impl.h"
#include "mobile/common/favorites/meta_info.h"

using bookmarks::BookmarkNode;

namespace mobile {

NodeFavoriteImpl::NodeFavoriteImpl(FavoritesImpl* favorites,
                                   int64_t parent,
                                   int64_t id,
                                   const BookmarkNode* data,
                                   const std::string& thumbnail_path)
    : Favorite(parent, id),
      favorites_(favorites),
      data_(data),
      thumbnail_path_(thumbnail_path) {
}

bool NodeFavoriteImpl::IsFolder() const {
  return false;
}

bool NodeFavoriteImpl::IsSavedPage() const {
  return false;
}

bool NodeFavoriteImpl::IsPartnerContent() const {
  return data()->HasAncestor(favorites_->local_root()->data()) &&
      MetaInfo::GetPushedPartnerChannel(data_);
}

bool NodeFavoriteImpl::CanChangeTitle() const {
  return true;
}

bool NodeFavoriteImpl::CanTransformToFolder() const {
  return parent() == favorites_->local_root()->id();
}

bool NodeFavoriteImpl::CanChangeParent() const {
  return true;
}

Folder* NodeFavoriteImpl::GetParent() const {
  return static_cast<Folder*>(favorites_->GetFavorite(parent()));
}

void NodeFavoriteImpl::Remove() {
  favorites_->RemoveNodeFavorite(this);
}

const std::string& NodeFavoriteImpl::guid() const {
  return MetaInfo::GetGUID(data_);
}

const base::string16& NodeFavoriteImpl::title() const {
  return data_->GetTitle();
}

void NodeFavoriteImpl::SetTitle(const base::string16& title) {
  favorites_->Rename(data_, title);
}

const GURL& NodeFavoriteImpl::url() const {
  return data_->url();
}

void NodeFavoriteImpl::SetURL(const GURL& url) {
  favorites_->ChangeURL(data_, url);
}

const std::string& NodeFavoriteImpl::thumbnail_path() const {
  return thumbnail_path_;
}

void NodeFavoriteImpl::SetThumbnailPath(const std::string& path) {
  if (thumbnail_path_.empty() && path.empty())
    return;
  thumbnail_path_ = path;
  favorites_->SignalChanged(
      id(), parent(), FavoritesObserver::THUMBNAIL_CHANGED);
}

void NodeFavoriteImpl::Activated() {
  if (PartnerId() != 0) {
    if (PartnerActivationCount() < 255) {
      SetPartnerActivationCount(PartnerActivationCount() + 1);
      favorites_->ReportPartnerContentEntryActivated(this);
    }
  }
}

int32_t NodeFavoriteImpl::PartnerActivationCount() const {
  return MetaInfo::GetPushedPartnerActivationCount(data_);
}

void NodeFavoriteImpl::SetPartnerActivationCount(int32_t activation_count) {
  favorites_->ChangePartnerActivationCount(data_, activation_count);
}

int32_t NodeFavoriteImpl::PartnerChannel() const {
  return MetaInfo::GetPushedPartnerChannel(data_);
}

void NodeFavoriteImpl::SetPartnerChannel(int32_t channel) {
  favorites_->ChangePartnerChannel(data_, channel);
}

int32_t NodeFavoriteImpl::PartnerId() const {
  return MetaInfo::GetPushedPartnerId(data_);
}

void NodeFavoriteImpl::SetPartnerId(int32_t id) {
  favorites_->ChangePartnerId(data_, id);
}

int32_t NodeFavoriteImpl::PartnerChecksum() const {
  return MetaInfo::GetPushedPartnerChecksum(data_);
}

void NodeFavoriteImpl::SetPartnerChecksum(int32_t checksum) {
  favorites_->ChangePartnerChecksum(data_, checksum);
}

const BookmarkNode* NodeFavoriteImpl::data() const {
  return data_;
}

}  // namespace mobile
