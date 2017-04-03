// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_FAVORITES_FOLDER_IMPL_H_
#define MOBILE_COMMON_FAVORITES_FOLDER_IMPL_H_

#include <string>
#include <vector>

#include "mobile/common/favorites/folder.h"

namespace mobile {

class FavoritesImpl;

class FolderImpl : public Folder {
 public:
  FolderImpl(FavoritesImpl* favorites,
             int64_t parent,
             int64_t id,
             const base::string16& title,
             const std::string& guid,
             const std::string& thumbnail_path);

  virtual ~FolderImpl();

  bool IsPartnerContent() const override;

  bool CanChangeTitle() const override;

  bool CanChangeParent() const override;

  Folder* GetParent() const override;

  const base::string16& title() const override;

  void SetTitle(const base::string16& title) override;

  const std::string& guid() const override;

  const std::string& thumbnail_path() const override;

  void SetThumbnailPath(const std::string& path) override;

  bool CanTakeMoreChildren() const override;

  int Size() const override;

  int TotalSize() override;

  int FoldersCount() override;

  const std::vector<int64_t>& Children() const override;

  Favorite* Child(int index) override;

  using Folder::Add;
  void Add(int pos, Favorite* favorite) override;

  void AddAll(Folder* folder) override;

  void Move(int old_position, int new_position) override;

  void ForgetAbout(Favorite* favorite) override;

  using Folder::IndexOf;
  int IndexOf(int64_t id) override;
  int IndexOf(const std::string& guid) override;

  int32_t PartnerGroup() const override;
  void SetPartnerGroup(int32_t group) override;

  void Activated() override;

 protected:
  virtual bool Check();

  void DisableCheck() override;
  void RestoreCheck() override;
  void RestoreCheckWithoutAction() override;

  FavoritesImpl* favorites_;
  base::string16 title_;
  std::string guid_;
  std::string thumbnail_path_;
  typedef std::vector<int64_t> children_t;
  children_t children_;
  int check_;

 private:
  DISALLOW_COPY_AND_ASSIGN(FolderImpl);
};

}  // namespace mobile

#endif  // MOBILE_COMMON_FAVORITES_FOLDER_IMPL_H_
