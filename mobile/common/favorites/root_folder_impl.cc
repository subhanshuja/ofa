// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/favorites/root_folder_impl.h"

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"

#include "components/bookmarks/browser/bookmark_node.h"

using bookmarks::BookmarkNode;

namespace mobile {

RootFolderImpl::RootFolderImpl(FavoritesImpl* favorites,
                               int64_t id,
                               const BookmarkNode* data)
    : DeviceRootFolderImpl(favorites, id, id, base::ASCIIToUTF16("[root]"), data) {
}

}  // namespace mobile
