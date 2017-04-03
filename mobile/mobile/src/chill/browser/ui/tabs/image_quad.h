// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_UI_TABS_IMAGE_QUAD_H_
#define CHILL_BROWSER_UI_TABS_IMAGE_QUAD_H_

#include <atomic>
#include <memory>
#include <mutex>

namespace opera {

class GLMipMap;
class GLTexture;

namespace tabui {

class TabSelectorRenderer;

// A renderable image.
//
// It holds a reference to the raw bitmap data, the OpenGL texture resource id
// and the shader parameters used to control it's appearance.
class ImageQuad {
 public:
  explicit ImageQuad(bool clamp);
  virtual ~ImageQuad();

  void Create(const std::shared_ptr<GLMipMap>& mipmap);

  void Update(const std::shared_ptr<GLMipMap>& mipmap);

  void OnGLContextLost();

  // Set target for number of levels that should be uploaded to gpu memory.
  void SetTargetLevels(size_t target_levels);

  void Upload(bool async);

  // Implements write-read fencing.
  // Release() is called in upload thread after glFlush() to make created
  // resources available in other threads. Acquire() is called in renderer
  // thread and will switch to resources flushed in previous frame.
  void Release();
  void Acquire();

  size_t id() const { return id_; }

  // Priority for next upload or -1 if no upload is required.
  int upload_priority() const;

  std::shared_ptr<GLMipMap> mipmap() const;
  std::shared_ptr<GLTexture> texture() const;

 private:
  int CalculateUploadLevels(bool async) const;

  // Counter for generating unique ids.
  static std::atomic<size_t> id_generator_;

  const size_t id_;
  const bool clamp_;

  std::shared_ptr<GLMipMap> mipmap_;
  std::shared_ptr<GLTexture> texture_;

  // If upload is needed.
  bool needs_upload_;

  // Actual number of uploaded levels.
  size_t current_levels_;

  // Target for number of uploaded levels.
  size_t target_levels_;

  // Guards access to internal variables.
  mutable std::mutex mutex_;
};

}  // namespace tabui
}  // namespace opera

#endif  // CHILL_BROWSER_UI_TABS_IMAGE_QUAD_H_
