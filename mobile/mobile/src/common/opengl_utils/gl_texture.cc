// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/opengl_utils/gl_texture.h"

#include "base/logging.h"
#include "base/memory/aligned_memory.h"
#include "third_party/skia/include/core/SkBitmap.h"

#include "common/opengl_utils/gl_library.h"
#include "common/opengl_utils/gl_mipmap.h"
#include "common/opengl_utils/opengl.h"

namespace opera {

// void glCheckError(const char* message) {
//   GLenum error = glGetError();
//   if (error != GL_NO_ERROR) {
//     LOG(ERROR) << "glCheckError failed: " << message;
//     switch (error) {
//     #define CASE(x) case x: LOG(ERROR) << #x; break;
//     CASE(GL_INVALID_ENUM)
//     CASE(GL_INVALID_VALUE)
//     CASE(GL_INVALID_OPERATION)
//     CASE(GL_INVALID_FRAMEBUFFER_OPERATION)
//     CASE(GL_OUT_OF_MEMORY)
//     default: LOG(ERROR) << "<unknown GL error>"; break;
//     }
//   }
// }

class GLTexture::Resource {
 public:
  Resource() : id_(0) {}

  ~Resource() { glDeleteTextures(1, &id_); }

  bool Activate() {
    if (eglGetCurrentContext() == EGL_NO_CONTEXT)
      return false;
    if (!id_)
      glGenTextures(1, &id_);
    if (!id_)
      return false;
    glBindTexture(GL_TEXTURE_2D, id_);
    return true;
  }

 private:
  GLuint id_;
};

GLTexture::GLTexture(Filter filter, bool clamp)
    : clamp_(clamp),
      filter_(filter),
      uploaded_resource_(new Resource()),
      released_resource_(uploaded_resource_),
      acquired_resource_(uploaded_resource_) {}

void GLTexture::Upload(const GLMipMap& image, bool mipmap, bool async) {
  // Create new resource id for the upload since the current one might be in use
  // for concurrent draw on renderer thread.
  std::shared_ptr<Resource> resource(new Resource());
  if (!resource->Activate())
    return;

  // We need to support uneven image sizes and smaller than 32 bit pixel size.
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  // There may be hardware restrictions for non-power-of-two textures.
  Filter filter = filter_;
  bool clamp = clamp_;

  GLLibrary& gl = GLLibrary::Get();
  if (!gl.SupportsNPOT()) {
    if (!SkIsPow2(image.width()) || !SkIsPow2(image.height())) {
      // Mip mapping isn't supported.
      if (filter_ == kMipMap)
        filter = kLinear;

      // Only clamp to edge is supported.
      clamp = true;
    }
  }
  if (!gl.SupportsMipmapping())
    mipmap = false;

  GLenum wrap = clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT;
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

  if (filter == kMipMap && !mipmap)
    filter = kLinear;

  // Tri-linear seems unnecessary when using mip maps, regular bi-linear should
  // be enough.
  GLenum min_filter;
  switch (filter) {
    case kMipMap:
      min_filter = GL_LINEAR_MIPMAP_NEAREST;
      break;
    case kLinear:
      min_filter = GL_LINEAR;
      break;
    default:
      min_filter = GL_NEAREST;
      break;
  }
  GLenum mag_filter = (filter == kNearest) ? GL_NEAREST : GL_LINEAR;
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);

  size_t num_levels = (filter == kMipMap) ? image.levels() : 1;

  // glCheckError("before upload");

  if (image.IsCompressed()) {
    GLenum format;
    switch (image.format()) {
      case GLMipMap::kATC:
        format = GL_ATC_RGB_AMD;
        break;
      case GLMipMap::kATCIA:
        format = GL_ATC_RGBA_INTERPOLATED_ALPHA_AMD;
        break;
      case GLMipMap::kDXT1:
        format = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
        break;
      case GLMipMap::kDXT5:
        format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        break;
      case GLMipMap::kETC1:
        format = GL_ETC1_RGB8_OES;
        break;
      default:
        LOG(ERROR) << "Unknown compressed image format in texture upload.";
        return;
    }

    for (size_t level = 0; level < num_levels; ++level) {
      glCompressedTexImage2D(GL_TEXTURE_2D, level, format, image.width(level),
                             image.height(level), 0, image.GetSize(level),
                             image.getPixels(level));

      // glCheckError("after compressed upload");
    }
  } else {
    for (size_t level = 0; level < num_levels; ++level) {
      glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, image.width(level),
                   image.height(level), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                   image.getPixels(level));

      // glCheckError("after upload");
    }
  }

  // Put the created resource as uploaded. If async, the new resource will not
  // be effective before gpu commands have been synchronized..
  std::unique_lock<std::mutex> scoped_lock(mutex_);
  uploaded_resource_ = resource;
  if (!async) {
    released_resource_ = resource;
    acquired_resource_ = resource;
  }
}

void GLTexture::Release() {
  std::unique_lock<std::mutex> scoped_lock(mutex_);
  released_resource_ = uploaded_resource_;
}

void GLTexture::Acquire() {
  std::unique_lock<std::mutex> scoped_lock(mutex_);
  acquired_resource_ = released_resource_;
}

void GLTexture::Activate() {
  std::unique_lock<std::mutex> scoped_lock(mutex_);
  acquired_resource_->Activate();
}

void GLTexture::Flush() {
  glFlush();
}

}  // namespace opera
