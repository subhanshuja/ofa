// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_UI_TABS_THUMBNAIL_CACHE_H_
#define CHILL_BROWSER_UI_TABS_THUMBNAIL_CACHE_H_

#include <jni.h>

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace opera {

class GLMipMap;

namespace tabui {

// In-memory cache for tab thumbnails.
//
// The ThumbnailCache maps from tab ids to GLMipmaps. When a tab id is updated
// with a new GLMipmap, the updated tab id is also assigned a generation number
// from a monotonically increasing generation counter. Generation numbers can
// then be used to request ids of tabs updated since a given point in time.
//
// The class is implemented as an opaque pointer to the real implementation,
// allowing it to be passed by value between C++ and Java.
class ThumbnailCache {
 public:
  // Creates a cache that will use a white default mipmap.
  ThumbnailCache();

  // Creates a cache that will use a specified default mipmap.
  explicit ThumbnailCache(jobject bitmap);

  // Adds a new tab id. Bumps generation counter.
  void AddTab(int id);

  // Removes a tab id and associated image. Bumps generation counter.
  void RemoveTab(int id);

  // Returns cached mipmap for a given tab id. If no bitmap was found a default
  // mipmap is returned if fallback is requested, otherwise a null pointer is
  // returned.
  std::shared_ptr<GLMipMap> GetMipmap(int id, bool fallback = true) const;

  // Adds an mipmap for a given tab id. The id must exist and it must not have
  // a mipmap since before.
  // Returns new generation number on success or -1 on failure.
  int64_t AddMipmap(int id, const std::shared_ptr<GLMipMap>& mipmap);

  // Updates mipmap for a given tab id. Bumps generation counter.
  // Returns new generation number on success or -1 on failure.
  int64_t UpdateMipmap(int id, const std::shared_ptr<GLMipMap>& mipmap);

  // Remove cached mipmaps for all tabs. Bumps generation count for each
  // affected tab.
  void Clear();

  // Remove cached mipmap for a given tab id. Bumps generation count for
  // affected tab.
  void Clear(int id);

  // Return pairs of generation number and id for tabs updated after a given
  // generation.
  std::vector<std::pair<int64_t, int>> GetChangedTabIds(
      int64_t generation) const;

  // Return value of generation counter.
  int64_t generation() const;

 private:
  class Impl;

  std::shared_ptr<Impl> impl_;
};

}  // namespace tabui
}  // namespace opera

#endif  // CHILL_BROWSER_UI_TABS_THUMBNAIL_CACHE_H_
