// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/favorites/special_folder_impl.h"

#include "mobile/common/favorites/favorites_impl.h"

using bookmarks::BookmarkNode;

namespace mobile {

SpecialFolderImpl::SpecialFolderImpl(FavoritesImpl* favorites,
                                     int64_t parent,
                                     int64_t id,
                                     int32_t special_id,
                                     const base::string16& title,
                                     const std::string& guid)
    : FolderImpl(favorites, parent, id, title, guid, ""),
      special_(special_id) {
  CHECK(special_ > 0);
}

const BookmarkNode* SpecialFolderImpl::data() const {
  return NULL;
}

void SpecialFolderImpl::Remove() {
  favorites_->RemoveNonNodeFavorite(this);
}

int32_t SpecialFolderImpl::GetSpecialId() const {
  return special_;
}

}  // namespace mobile
