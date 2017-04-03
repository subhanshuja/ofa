// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/ui/tabs/thumbnail_cache.h"

#include <algorithm>
#include <map>
#include <mutex>
#include <unordered_map>

#include "base/logging.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"

#include "common/opengl_utils/gl_mipmap.h"

namespace opera {
namespace tabui {

class ThumbnailCache::Impl {
 public:
  Impl() : default_mipmap_(CreateDefaultMipmap()), generation_(0) {}

  explicit Impl(jobject bitmap)
      : default_mipmap_(
            GLMipMap::Create(bitmap, false, GLMipMap::kCompressAlways)),
        generation_(0) {}

  void AddTab(int id) {
    MipmapRecord record;
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    record.generation = ++generation_;
    id_map_.emplace(id, std::move(record));
    generation_map_.emplace_hint(generation_map_.end(), generation_, id);
    DCHECK_EQ(id_map_.size(), generation_map_.size());
  }

  void RemoveTab(int id) {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    auto it = id_map_.find(id);
    DCHECK(it != id_map_.end());
    generation_map_.erase(it->second.generation);
    id_map_.erase(it);
    ++generation_;
    DCHECK_EQ(id_map_.size(), generation_map_.size());
  }

  // Get mipmap for tab id.
  std::shared_ptr<GLMipMap> GetMipmap(int id, bool fallback) {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    auto it = id_map_.find(id);
    if (it == id_map_.end() || !it->second.mipmap)
      return fallback ? default_mipmap_ : std::shared_ptr<GLMipMap>();
    return it->second.mipmap;
  }

  int64_t AddMipmap(int id, const std::shared_ptr<GLMipMap>& mipmap) {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    auto it = id_map_.find(id);
    if (it == id_map_.end())
      return -1;
    if (it->second.mipmap)
      return -1;
    return UpdateInternal(it, mipmap);
  }

  // Set mipmap for an existing tab.
  int64_t UpdateMipMap(int id, const std::shared_ptr<GLMipMap>& mipmap) {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    auto it = id_map_.find(id);
    if (it == id_map_.end())
      return -1;
    return UpdateInternal(it, mipmap);
  }

  void Clear() {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    for (auto it = id_map_.begin(); it != id_map_.end(); ++it) {
      if (it->second.mipmap)
        UpdateInternal(it, std::shared_ptr<GLMipMap>());
    }
  }

  std::vector<std::pair<int64_t, int>> GetChangedTabIds(int64_t generation) {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    std::vector<std::pair<int64_t, int>> result;
    copy(generation_map_.upper_bound(generation), generation_map_.end(),
         back_inserter(result));
    return result;
  }

  int64_t generation() {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    return generation_;
  }

 private:
  struct MipmapRecord {
    std::shared_ptr<GLMipMap> mipmap;
    int64_t generation;
  };

  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;

  int64_t UpdateInternal(std::unordered_map<int, MipmapRecord>::iterator it,
                         const std::shared_ptr<GLMipMap>& mipmap) {
    generation_map_.erase(it->second.generation);
    it->second.mipmap = mipmap;
    it->second.generation = ++generation_;
    generation_map_.emplace_hint(generation_map_.end(), generation_, it->first);
    DCHECK_EQ(id_map_.size(), generation_map_.size());
    return generation_;
  }

  static std::shared_ptr<GLMipMap> CreateDefaultMipmap();

  // Mipmap to use when no mipmap has ben set.
  const std::shared_ptr<GLMipMap> default_mipmap_;

  // Guards access to internal data members.
  std::mutex mutex_;

  // Generation counter that is incremented at each update.
  int64_t generation_;

  // Maps from id to mipmap record.
  std::unordered_map<int, MipmapRecord> id_map_;

  // Maps from generation to id.
  std::map<int64_t, int> generation_map_;
};

ThumbnailCache::ThumbnailCache() : impl_(new Impl()) {}

ThumbnailCache::ThumbnailCache(jobject bitmap) : impl_(new Impl(bitmap)) {}

void ThumbnailCache::AddTab(int id) {
  impl_->AddTab(id);
}

void ThumbnailCache::RemoveTab(int id) {
  impl_->RemoveTab(id);
}

std::shared_ptr<GLMipMap> ThumbnailCache::GetMipmap(int id,
                                                    bool fallback) const {
  return impl_->GetMipmap(id, fallback);
}

int64_t ThumbnailCache::AddMipmap(int id,
                                  const std::shared_ptr<GLMipMap>& mipmap) {
  DCHECK(mipmap);
  return impl_->AddMipmap(id, mipmap);
}

int64_t ThumbnailCache::UpdateMipmap(int id,
                                     const std::shared_ptr<GLMipMap>& mipmap) {
  DCHECK(mipmap);
  return impl_->UpdateMipMap(id, mipmap);
}

std::vector<std::pair<int64_t, int>> ThumbnailCache::GetChangedTabIds(
    int64_t generation) const {
  return impl_->GetChangedTabIds(generation);
}

void ThumbnailCache::Clear() {
  impl_->Clear();
}

void ThumbnailCache::Clear(int id) {
  impl_->UpdateMipMap(id, nullptr);
}

int64_t ThumbnailCache::generation() const {
  return impl_->generation();
}

std::shared_ptr<GLMipMap> ThumbnailCache::Impl::CreateDefaultMipmap() {
  SkBitmap bitmap;
  if (!bitmap.tryAllocPixels(SkImageInfo::MakeN32Premul(16, 16))) {
    // Can not happen on android since malloc (almost) never fails.
    NOTREACHED();
  }
  bitmap.eraseARGB(255, 255, 255, 255);
  return GLMipMap::Create(bitmap, false, GLMipMap::kCompressAlways);
}

}  // namespace tabui
}  // namespace opera
