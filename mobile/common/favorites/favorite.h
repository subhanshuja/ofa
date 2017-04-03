// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_FAVORITES_FAVORITE_H_
#define MOBILE_COMMON_FAVORITES_FAVORITE_H_

#include <string>

#include "base/strings/string16.h"
#include "url/gurl.h"

namespace bookmarks {
class BookmarkNode;
}

namespace mobile {

class Folder;

class Favorite {
 public:
  enum ThumbnailMode {
    DOWNLOAD,  // Favorite wants a downloaded thumbnail
    FALLBACK,  // Favorite only wants to use fallback thumbnail
    NONE,      // Favorite does not want a thumbnail
  };
  virtual ~Favorite() {}

  // True if the favorite is a folder (Folder).
  // url() is always empty
  virtual bool IsFolder() const = 0;
  // True if the favorite is a saved page (SavedPage).
  // Can not be a folder
  virtual bool IsSavedPage() const = 0;
  // True if the favorite is a partner content (pushed content)
  // Can be a folder
  virtual bool IsPartnerContent() const = 0;
  // True if it is valid to call SetTitle
  virtual bool CanChangeTitle() const = 0;
  // True if the Favorite can be merged with another Favorite (that also has
  // CanTransformToFolder() == true) to create a folder
  virtual bool CanTransformToFolder() const = 0;
  // True if the Favorite may change parent or in case of Folders be merged
  // with another Folder
  virtual bool CanChangeParent() const = 0;

  virtual ThumbnailMode GetThumbnailMode() const {
    return ThumbnailMode::DOWNLOAD;
  }

  // Session id, not kept between restarts
  int64_t id() const { return id_; }

  int64_t parent() const { return parent_; }

  virtual Folder* GetParent() const = 0;

  virtual void Remove() = 0;

  // Stable id
  virtual const std::string& guid() const = 0;

  virtual const base::string16& title() const = 0;

  virtual void SetTitle(const base::string16& title) = 0;

  virtual const GURL& url() const = 0;

  virtual void SetURL(const GURL& url) = 0;

  virtual const std::string& thumbnail_path() const = 0;

  virtual void SetThumbnailPath(const std::string& thumbnail_path) = 0;

  virtual void Activated() = 0;

  virtual int32_t PartnerActivationCount() const = 0;
  virtual void SetPartnerActivationCount(int32_t activation_count) = 0;

  virtual int32_t PartnerChannel() const = 0;
  virtual void SetPartnerChannel(int32_t channel) = 0;

  virtual int32_t PartnerId() const = 0;
  virtual void SetPartnerId(int32_t id) = 0;

  virtual int32_t PartnerChecksum() const = 0;
  virtual void SetPartnerChecksum(int32_t checksum) = 0;

  virtual const bookmarks::BookmarkNode* data() const = 0;

  virtual int32_t GetSpecialId() const { return 0; }

  virtual bool HasAncestor(const Folder* folder) const;
  // Returns true if this favorite matches id or if any parent of this
  // favorite matches id
  virtual bool HasAncestor(int64_t id) const;

 protected:
  friend class Folder;

  Favorite(int64_t parent, int64_t id) : parent_(parent), id_(id) {}

  void set_parent(int64_t parent) { parent_ = parent; }

 private:
  int64_t parent_, id_;

  DISALLOW_COPY_AND_ASSIGN(Favorite);
};

}  // namespace mobile

#endif  // MOBILE_COMMON_FAVORITES_FAVORITE_H_
