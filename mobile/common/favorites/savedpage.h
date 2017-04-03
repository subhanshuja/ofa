// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_FAVORITES_SAVEDPAGE_H_
#define MOBILE_COMMON_FAVORITES_SAVEDPAGE_H_

#include <string>

#include "mobile/common/favorites/favorite.h"

namespace mobile {

class SavedPage : public Favorite {
 public:
  bool IsFolder() const override;
  bool IsSavedPage() const override;
  bool IsPartnerContent() const override;
  bool CanTransformToFolder() const override;
  bool CanChangeParent() const override;

  int32_t PartnerActivationCount() const override;
  void SetPartnerActivationCount(int32_t activation_count) override;
  int32_t PartnerChannel() const override;
  void SetPartnerChannel(int32_t channel) override;
  int32_t PartnerId() const override;
  void SetPartnerId(int32_t id) override;
  int32_t PartnerChecksum() const override;
  void SetPartnerChecksum(int32_t checksum) override;

  const bookmarks::BookmarkNode* data() const override { return NULL; }

  virtual const std::string& file() const = 0;

  virtual void SetFile(const std::string& file, bool force) = 0;
  virtual void SetFile(const std::string& file) = 0;

 protected:
  SavedPage(int64_t parent, int64_t id) : Favorite(parent, id) {}

 private:
  DISALLOW_COPY_AND_ASSIGN(SavedPage);
};

}  // namespace mobile

#endif  // MOBILE_COMMON_FAVORITES_SAVEDPAGE_H_
