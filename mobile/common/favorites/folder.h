// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_FAVORITES_FOLDER_H_
#define MOBILE_COMMON_FAVORITES_FOLDER_H_

#include <string>
#include <vector>

#include "base/time/time.h"
#include "mobile/common/favorites/favorite.h"

namespace mobile {

class Folder : public Favorite {
 public:
  bool IsFolder() const override;
  bool IsSavedPage() const override;
  bool CanTransformToFolder() const override;
  // True if the folder allows more Favorites to be added
  virtual bool CanTakeMoreChildren() const = 0;

  ThumbnailMode GetThumbnailMode() const override {
    return ThumbnailMode::NONE;
  }

  const GURL& url() const override;

  void SetURL(const GURL& url) override;

  // Number of Favorites in this folder
  virtual int Size() const = 0;

  // Number of Favorites in this and sub folders
  virtual int TotalSize() = 0;

  // Number of folders in this and sub folders
  virtual int FoldersCount() = 0;

  virtual const std::vector<int64_t>& Children() const = 0;

  virtual Favorite* Child(int index) = 0;

  virtual void Add(Favorite* favorite);

  virtual void Add(int pos, Favorite* favorite) = 0;

  virtual void AddAll(Folder* folder) = 0;

  // new_position is calculated as if old_position is already removed,
  // so {A, B, C} Move(0, 1) will result in {B, A, C}
  virtual void Move(int old_position, int new_position) = 0;

  // Remove the favorite mapping in the folder. This shouldn't delete the
  // actual favorite, that should be handled by the caller
  virtual void ForgetAbout(Favorite* favorite) = 0;

  // Returns -1 if favorite isn't a child of folder
  virtual int IndexOf(Favorite* favorite);
  // Returns -1 if there is no child of folder with given id
  virtual int IndexOf(int64_t id) = 0;
  // Returns -1 if there is no child of folder with given guid
  virtual int IndexOf(const std::string& guid) = 0;

  // Partner content methods. Folders only have valid partner groups
  virtual int32_t PartnerGroup() const = 0;
  virtual void SetPartnerGroup(int32_t group) = 0;

  int32_t PartnerActivationCount() const override;
  void SetPartnerActivationCount(int32_t activation_count) override;

  int32_t PartnerChannel() const override;
  void SetPartnerChannel(int32_t channel) override;

  int32_t PartnerId() const override;
  void SetPartnerId(int32_t id) override;

  int32_t PartnerChecksum() const override;
  void SetPartnerChecksum(int32_t checksum) override;

  virtual bool Check() = 0;

  virtual const base::Time& last_modified() const;

 protected:
  friend class FavoritesImpl;

  virtual void DisableCheck() = 0;
  virtual void RestoreCheck() = 0;
  virtual void RestoreCheckWithoutAction() = 0;

  Folder(int64_t parent, int64_t id) : Favorite(parent, id) {}

  void changeParentToMe(Favorite* favorite) { favorite->set_parent(id()); }

 private:
  DISALLOW_COPY_AND_ASSIGN(Folder);
};

}  // namespace mobile

#endif  // MOBILE_COMMON_FAVORITES_FOLDER_H_
