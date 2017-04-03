// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/favorites/node_folder_impl.h"

#include "components/bookmarks/browser/bookmark_node.h"

#include "mobile/common/favorites/favorites_impl.h"
#include "mobile/common/favorites/meta_info.h"

using bookmarks::BookmarkNode;

namespace mobile {

NodeFolderImpl::NodeFolderImpl(FavoritesImpl* favorites,
                               int64_t parent,
                               int64_t id,
                               const BookmarkNode* data,
                               const std::string& thumbnail_path)
    : FolderImpl(favorites,
                 parent,
                 id,
                 data->GetTitle(),
                 MetaInfo::GetGUID(data),
                 thumbnail_path),
      data_(data) {
}

bool NodeFolderImpl::IsPartnerContent() const {
  return data()->HasAncestor(favorites_->local_root()->data()) &&
      MetaInfo::GetPushedPartnerGroupId(data_) != 0;
}

const base::string16& NodeFolderImpl::title() const {
  return data_->GetTitle();
}

void NodeFolderImpl::SetTitle(const base::string16& title) {
  favorites_->Rename(data_, title);
}

const BookmarkNode* NodeFolderImpl::data() const {
  return data_;
}

int32_t NodeFolderImpl::PartnerGroup() const {
  return MetaInfo::GetPushedPartnerGroupId(data_);
}

void NodeFolderImpl::SetPartnerGroup(int32_t group) {
  favorites_->ChangePartnerGroup(data_, group);
}

void NodeFolderImpl::Remove() {
  favorites_->RemoveNodeFavorite(this);
}

}  // namespace mobile
