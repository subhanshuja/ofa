// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/ui/tabs/image_quad.h"

#include <algorithm>
#include <limits>

#include "base/logging.h"

#include "common/opengl_utils/gl_mipmap.h"
#include "common/opengl_utils/gl_texture.h"

namespace opera {
namespace tabui {

ImageQuad::ImageQuad(bool clamp)
    : id_(++id_generator_),
      clamp_(clamp),
      needs_upload_(false),
      current_levels_(0),
      target_levels_(0) {}

ImageQuad::~ImageQuad() {}

void ImageQuad::Create(const std::shared_ptr<GLMipMap>& mipmap) {
  std::unique_lock<std::mutex> scoped_lock(mutex_);

  mipmap_ = mipmap;
  texture_ = std::make_shared<GLTexture>(GLTexture::kMipMap, clamp_);
  needs_upload_ = true;
  current_levels_ = 0;
  target_levels_ = mipmap->levels();
}

void ImageQuad::Update(const std::shared_ptr<GLMipMap>& mipmap) {
  std::unique_lock<std::mutex> scoped_lock(mutex_);

  mipmap_ = mipmap;
  needs_upload_ = true;
  target_levels_ = mipmap->levels();
}

void ImageQuad::OnGLContextLost() {
  std::unique_lock<std::mutex> scoped_lock(mutex_);

  texture_.reset();
  needs_upload_ = true;
  current_levels_ = 0;
}

void ImageQuad::SetTargetLevels(size_t target_levels) {
  target_levels_ = std::min(mipmap_->levels(), target_levels);
  if (target_levels_ != current_levels_)
    needs_upload_ = true;
}

void ImageQuad::Upload(bool async) {
  DCHECK(mipmap_);

  int levels;
  std::unique_ptr<GLMipMap> upload_mipmap;
  std::shared_ptr<GLTexture> upload_texture;
  bool mipmap;
  {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    if (!needs_upload_)
      return;

    levels = CalculateUploadLevels(async);
    if (!texture_ || levels < 0)
      return;

    upload_mipmap = mipmap_->CreateReduced(levels);
    upload_texture = texture_;

    // Use mipmap in final upload only.
    mipmap = levels == static_cast<int>(target_levels_);
    current_levels_ = levels;
    needs_upload_ = current_levels_ != target_levels_;
  }

  upload_texture->Upload(*upload_mipmap, mipmap, async);
}

void ImageQuad::Release() {
  std::unique_lock<std::mutex> scoped_lock(mutex_);
  texture_->Release();
}

void ImageQuad::Acquire() {
  std::unique_lock<std::mutex> scoped_lock(mutex_);
  texture_->Acquire();
}

// Priority will be negative if current number of levels is below target, or
// positive if above target. Thus blurry image quads will be serverd first to
// make them sharper, while already sharp images that should reduce number of
// uploaded levels should be served last in order. A min-int priority indicates
// target is reached and no more upload is required for this image quad.
int ImageQuad::upload_priority() const {
  std::unique_lock<std::mutex> scoped_lock(mutex_);
  if (!needs_upload_)
    return std::numeric_limits<int>::min();

  return current_levels_ - target_levels_;
}

std::shared_ptr<GLMipMap> ImageQuad::mipmap() const {
  std::unique_lock<std::mutex> scoped_lock(mutex_);
  return mipmap_;
}

std::shared_ptr<GLTexture> ImageQuad::texture() const {
  std::unique_lock<std::mutex> scoped_lock(mutex_);
  return texture_;
}

// Calculate levels to use in next upload. Returns -1 if no upload is needed.
int ImageQuad::CalculateUploadLevels(bool async) const {
  if (!needs_upload_)
    return -1;

  if (async && current_levels_ < target_levels_)
    return current_levels_ + 1;

  return target_levels_;
}

std::atomic<size_t> ImageQuad::id_generator_;

}  // namespace tabui
}  // namespace opera
