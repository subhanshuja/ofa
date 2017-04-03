// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_FAVORITES_DEVICE_ROOT_FOLDER_IMPL_H
#define MOBILE_COMMON_FAVORITES_DEVICE_ROOT_FOLDER_IMPL_H

#include "mobile/common/favorites/folder_impl.h"

namespace mobile {

class FavoritesImpl;

class DeviceRootFolderImpl : public FolderImpl {
 public:
  DeviceRootFolderImpl(FavoritesImpl* favorites,
                       int64_t parent,
                       int64_t id,
                       const base::string16& device_name,
                       const bookmarks::BookmarkNode* data);

  bool CanChangeTitle() const override;

  const base::string16& title() const override;

  void SetTitle(const base::string16& title) override;

  void Remove() override;

  const bookmarks::BookmarkNode* data() const override;

  void SetData(const bookmarks::BookmarkNode* data);

 protected:
  bool Check() override;

 private:
  const bookmarks::BookmarkNode* data_;

  DISALLOW_COPY_AND_ASSIGN(DeviceRootFolderImpl);
};

}  // namespace mobile

#endif  // MOBILE_COMMON_FAVORITES_DEVICE_ROOT_FOLDER_IMPL_H_
