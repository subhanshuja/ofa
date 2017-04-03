// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/favorites/device_root_folder_impl.h"

#include "base/logging.h"

#include "components/bookmarks/browser/bookmark_node.h"

using bookmarks::BookmarkNode;

namespace mobile {

DeviceRootFolderImpl::DeviceRootFolderImpl(FavoritesImpl* favorites,
                                           int64_t parent,
                                           int64_t id,
                                           const base::string16& device_name,
                                           const BookmarkNode* data)
    : FolderImpl(favorites, parent, id, device_name, "", ""),
      data_(data) {
}

bool DeviceRootFolderImpl::CanChangeTitle() const {
  return false;
}

const base::string16& DeviceRootFolderImpl::title() const {
  return data_ ? data_->GetTitle() : FolderImpl::title();
}

void DeviceRootFolderImpl::SetTitle(const base::string16& title) {
  NOTREACHED();
}

void DeviceRootFolderImpl::Remove() {
  NOTREACHED();
}

bool DeviceRootFolderImpl::Check() {
  return false;
}

const BookmarkNode* DeviceRootFolderImpl::data() const {
  return data_;
}

void DeviceRootFolderImpl::SetData(const BookmarkNode* data) {
  data_ = data;
}

}  // namespace mobile
