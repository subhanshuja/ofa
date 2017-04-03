// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_FAVORITES_NODE_FOLDER_IMPL_H_
#define MOBILE_COMMON_FAVORITES_NODE_FOLDER_IMPL_H_

#include <string>

#include "mobile/common/favorites/folder_impl.h"

namespace mobile {

class NodeFolderImpl : public FolderImpl {
 public:
  NodeFolderImpl(FavoritesImpl* favorites,
                 int64_t parent,
                 int64_t id,
                 const bookmarks::BookmarkNode* data,
                 const std::string& thumbnail_path);

  bool IsPartnerContent() const override;

  const base::string16& title() const override;

  void SetTitle(const base::string16& title) override;

  int32_t PartnerGroup() const override;
  void SetPartnerGroup(int32_t group) override;

  const bookmarks::BookmarkNode* data() const override;

  void Remove() override;

 protected:
  const bookmarks::BookmarkNode* data_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NodeFolderImpl);
};

}  // namespace mobile

#endif  // MOBILE_COMMON_FAVORITES_NODE_FOLDER_IMPL_H_
