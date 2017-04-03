// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RASTER_TEXTURE_COMPRESSOR_DXT_H_
#define CC_RASTER_TEXTURE_COMPRESSOR_DXT_H_

#include "cc/raster/texture_compressor.h"

namespace cc {

class CC_EXPORT TextureCompressorDXT : public TextureCompressor {
 private:
  bool supports_opacity_;

 public:
  /**
   * Creates a DXT encoder. It may create either a DXT1 or DXT5 encoder,
   * depending on whether opacity support is needed.
   */
  explicit TextureCompressorDXT(bool supports_opacity) :
      supports_opacity_(supports_opacity) {}

  void Compress(const uint8_t* src,
                uint8_t* dst,
                int width,
                int height,
                Quality quality) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TextureCompressorDXT);
};

}  // namespace cc

#endif  // CC_RASTER_TEXTURE_COMPRESSOR_DXT_H_
