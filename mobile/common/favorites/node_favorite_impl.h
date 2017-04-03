// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_FAVORITES_NODE_FAVORITE_IMPL_H_
#define MOBILE_COMMON_FAVORITES_NODE_FAVORITE_IMPL_H_

#include <string>

#include "mobile/common/favorites/favorite.h"

namespace bookmarks {
class BookmarkNode;
}

namespace mobile {

class FavoritesImpl;

class NodeFavoriteImpl : public Favorite {
 public:
  NodeFavoriteImpl(FavoritesImpl* favorites,
                   int64_t parent,
                   int64_t id,
                   const bookmarks::BookmarkNode* data,
                   const std::string& thumbnail_path);

  bool IsFolder() const override;

  bool IsSavedPage() const override;

  bool IsPartnerContent() const override;

  bool CanChangeTitle() const override;

  bool CanTransformToFolder() const override;

  bool CanChangeParent() const override;

  Folder* GetParent() const override;

  void Remove() override;

  const std::string& guid() const override;

  const base::string16& title() const override;

  void SetTitle(const base::string16& title) override;

  const GURL& url() const override;

  void SetURL(const GURL& url) override;

  const std::string& thumbnail_path() const override;

  void SetThumbnailPath(const std::string& path) override;

  void Activated() override;

  int32_t PartnerActivationCount() const override;
  void SetPartnerActivationCount(int32_t activation_count) override;

  int32_t PartnerChannel() const override;
  void SetPartnerChannel(int32_t channel) override;

  int32_t PartnerId() const override;
  void SetPartnerId(int32_t id) override;

  int32_t PartnerChecksum() const override;
  void SetPartnerChecksum(int32_t checksum) override;

  const bookmarks::BookmarkNode* data() const override;

 protected:
  FavoritesImpl* favorites_;
  const bookmarks::BookmarkNode* data_;
  std::string thumbnail_path_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NodeFavoriteImpl);
};

}  // namespace mobile

#endif  // MOBILE_COMMON_FAVORITES_NODE_FAVORITE_IMPL_H_
