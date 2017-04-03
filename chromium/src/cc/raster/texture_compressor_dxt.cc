// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/raster/texture_compressor_dxt.h"

#include "base/logging.h"
#include "cc/raster/dxt_encoder.h"

namespace cc {

void TextureCompressorDXT::Compress(const uint8_t* src,
                                    uint8_t* dst,
                                    int width,
                                    int height,
                                    Quality quality) {
  CompressDXT(src, dst, width, height, !supports_opacity_, quality);
}

}  // namespace cc
