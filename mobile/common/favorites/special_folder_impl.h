// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_FAVORITES_SPECIAL_FOLDER_IMPL_H_
#define MOBILE_COMMON_FAVORITES_SPECIAL_FOLDER_IMPL_H_

#include <string>

#include "mobile/common/favorites/folder_impl.h"

namespace mobile {

class SpecialFolderImpl : public FolderImpl {
 public:
  SpecialFolderImpl(FavoritesImpl* favorites,
                    int64_t parent,
                    int64_t id,
                    int32_t special_id,
                    const base::string16& title,
                    const std::string& guid);

  const bookmarks::BookmarkNode* data() const override;

  void Remove() override;

  int32_t GetSpecialId() const override;

 protected:
  const int32_t special_;

 private:
  DISALLOW_COPY_AND_ASSIGN(SpecialFolderImpl);
};

}  // namespace mobile

#endif  // MOBILE_COMMON_FAVORITES_NODE_FOLDER_IMPL_H_
