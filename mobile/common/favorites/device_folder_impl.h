// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_FAVORITES_DEVICE_FOLDER_IMPL_H_
#define MOBILE_COMMON_FAVORITES_DEVICE_FOLDER_IMPL_H_

#include "mobile/common/favorites/node_folder_impl.h"

namespace bookmarks {
class BookmarkNode;
}

namespace mobile {

class FavoritesImpl;

class DeviceFolderImpl : public NodeFolderImpl {
 public:
  DeviceFolderImpl(FavoritesImpl* favorites,
                   int64_t parent,
                   int64_t id,
                   const bookmarks::BookmarkNode* data);

 protected:
  virtual bool Check() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DeviceFolderImpl);
};

}  // namespace mobile

#endif  // MOBILE_COMMON_FAVORITES_DEVICE_FOLDER_IMPL_H_
