// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/favorites/device_folder_impl.h"

#include "components/bookmarks/browser/bookmark_node.h"
#include "mobile/common/favorites/favorites_impl.h"

namespace mobile {

DeviceFolderImpl::DeviceFolderImpl(FavoritesImpl* favorites,
                                   int64_t parent,
                                   int64_t id,
                                   const bookmarks::BookmarkNode* data)
    : NodeFolderImpl(favorites, parent, id, data, "") {
}

bool DeviceFolderImpl::Check() {
  return false;
}

}  // namespace mobile
