// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OPENGL_UTILS_GL_TEXTURE_H_
#define COMMON_OPENGL_UTILS_GL_TEXTURE_H_

#include <memory>
#include <mutex>

namespace opera {

class GLMipMap;

// Encapsulates a two-dimensional image that can be used when rendering geometry
// in OpenGL.
class GLTexture {
 public:
  // The access patterns used when the rasterizer asks for a texel.
  enum Filter {
    // Find the closest match.
    kNearest,
    // Find the four closest matches and do a weighted blend between them.
    kLinear,
    // Find the closest mip image and do a linear blend on that image.
    // Note that this isn't tri-linear filtering.
    kMipMap
  };

  // Create an object for the image data on the graphics card.
  //
  // |queue| TODO
  //
  // |filter| tells the object if it pixel access should be a filtered average
  // or the closest texel. Filtering improves visual quality if the image isn't
  // drawn in a 1-to-1 between the displayed and actual size.
  //
  // |clamp| tells the object if the borders should be considered. If it's true
  // then any texture coordinate outside 0..1 will be clamped to the edges. If
  // it's set to false then the coordinates are wrapped and it results in a
  // tiled image.
  //
  // Note that filtering and clamp should be considered mere requests.
  // They may be changed if the hardware doesn't support the requested
  // combination.
  explicit GLTexture(Filter filter = kNearest, bool clamp = true);

  // Upload image data to gpu memory. If async, the new texuture will not be
  // available before next Release()/Acquire() cycle.
  void Upload(const GLMipMap& image, bool mipmap, bool async);

  // Implements write-read fencing.
  // Release() is called in upload thread after glFlush() to make created
  // resources available in other threads. Acquire() is called in renderer
  // thread
  // and will switch to resources flushed in previous frame.
  void Release();
  void Acquire();

  // Sets this texture object as active in the current GL context. This needs
  // to be done before any geometry is rendered.
  void Activate();

  static void Flush();

 private:
  // Wrapper for texture id.
  class Resource;

  const bool clamp_;
  const Filter filter_;

  // Guards access to internal members.
  std::mutex mutex_;

  // Resource id for uploaded texture.
  std::shared_ptr<Resource> uploaded_resource_;

  // Resource id for released texture.
  std::shared_ptr<Resource> released_resource_;

  // Resource id for acquired texture.
  std::shared_ptr<Resource> acquired_resource_;
};

}  // namespace opera

#endif  // COMMON_OPENGL_UTILS_GL_TEXTURE_H_
