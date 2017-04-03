// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OPENGL_UTILS_GL_LIBRARY_H_
#define COMMON_OPENGL_UTILS_GL_LIBRARY_H_

#include <set>
#include <string>

#include "base/macros.h"

namespace opera {

// Handles checking for and initialization of OpenGL extensions as well as some
// generic utility methods.
class GLLibrary {
 public:
  static GLLibrary& Get();

  // Check for extension support and initialize the ones we care about.
  bool Init();
  bool initialized() const { return initialized_; }

  // Set up the output rect for the render buffer.
  void Viewport(int width, int height);

  // Clear the color buffer to a specific color.
  void Clear(const float rgba[4]);

  // Enable alpha blending for source data.
  void EnableBlend();

  // Check for available extensions.
  bool SupportsNPOT() const { return npot_; }
  bool SupportsVAO() const { return vertex_array_objects_; }

  // Check for available texture compression extensions.
  bool SupportsATC() const { return texture_compression_.atc_; }
  bool SupportsDXT1() const {
    return texture_compression_.dxt1_ || texture_compression_.s3tc_;
  }
  bool SupportsDXT5() const { return texture_compression_.s3tc_; }
  bool SupportsETC1() const { return texture_compression_.etc1_; }

  bool SupportsMipmapping() const { return mipmapping_; }
  bool SupportsAsyncUpload() const { return async_upload_; }

  bool SupportsShaderTextureLOD() const { return shader_texture_lod_; }

 private:
  struct TextureCompression {
    unsigned atc_ : 1;
    unsigned dxt1_ : 1;
    unsigned etc1_ : 1;
    unsigned s3tc_ : 1;

    TextureCompression()
        : atc_(false), dxt1_(false), etc1_(false), s3tc_(false) {}
  };

  // If true, then the current context has been examined and supported extension
  // etc has been set up.
  bool initialized_;

  // True if the hardware support Non Power Of Two textures beyond what
  // OpenGL ES 2.0 requires like mip mapping and edge wrapping modes.
  bool npot_;

  // True if the hardware supports combining all geometry states into one
  // object.
  bool vertex_array_objects_;

  // True unless the drivers are broken when dealing with mipmap textures.
  bool mipmapping_;

  // True unless the drivers are broken when uploading on a different thread.
  bool async_upload_;

  // True if the driver supports shader texture lod extension.
  bool shader_texture_lod_;

  // Which compression algorithms are supported.
  TextureCompression texture_compression_;

  GLLibrary();

  DISALLOW_COPY_AND_ASSIGN(GLLibrary);
};

}  // namespace opera

#endif  // COMMON_OPENGL_UTILS_GL_LIBRARY_H_
