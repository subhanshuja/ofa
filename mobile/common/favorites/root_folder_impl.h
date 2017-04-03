// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_FAVORITES_ROOT_FOLDER_IMPL_H_
#define MOBILE_COMMON_FAVORITES_ROOT_FOLDER_IMPL_H_

#include "mobile/common/favorites/device_root_folder_impl.h"

namespace mobile {

class RootFolderImpl : public DeviceRootFolderImpl {
 public:
  RootFolderImpl(FavoritesImpl* favorites,
                 int64_t id,
                 const bookmarks::BookmarkNode* data);

  DISALLOW_COPY_AND_ASSIGN(RootFolderImpl);
};

}  // namespace mobile

#endif  // MOBILE_COMMON_FAVORITES_ROOT_FOLDER_IMPL_H_
