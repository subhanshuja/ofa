// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_FAVORITES_SAVEDPAGE_IMPL_H_
#define MOBILE_COMMON_FAVORITES_SAVEDPAGE_IMPL_H_

#include <string>

#include "mobile/common/favorites/savedpage.h"

namespace mobile {

class FavoritesImpl;

class SavedPageImpl : public SavedPage {
 public:
  SavedPageImpl(FavoritesImpl* favorites,
                int64_t parent,
                int64_t id,
                const base::string16& title,
                const GURL& url,
                const std::string& guid,
                const std::string& file,
                const std::string& thumbnail);
  virtual ~SavedPageImpl();

  bool CanChangeTitle() const override;

  Favorite::ThumbnailMode GetThumbnailMode() const override;

  Folder* GetParent() const override;

  void Remove() override;

  const std::string& guid() const override;

  const base::string16& title() const override;

  void SetTitle(const base::string16& title) override;

  const GURL& url() const override;

  void SetURL(const GURL& url) override;

  const std::string& thumbnail_path() const override;

  void SetThumbnailPath(const std::string& path) override;

  const std::string& file() const override;

  void SetFile(const std::string& file, bool force) override;

  void SetFile(const std::string& file) override;

  void Activated() override;

 protected:
  FavoritesImpl* favorites_;
  base::string16 title_;
  GURL url_;
  std::string thumbnail_path_;
  std::string guid_, file_;

 private:
  DISALLOW_COPY_AND_ASSIGN(SavedPageImpl);
};

}  // namespace mobile

#endif  // MOBILE_COMMON_FAVORITES_SAVEDPAGE_IMPL_H_
