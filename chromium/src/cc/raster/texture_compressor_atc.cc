// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/raster/texture_compressor_atc.h"

#include "base/logging.h"
#include "cc/raster/dxt_encoder.h"

namespace cc {

void TextureCompressorATC::Compress(const uint8_t* src,
                                    uint8_t* dst,
                                    int width,
                                    int height,
                                    Quality quality) {
  CompressATC(src, dst, width, height, !supports_opacity_, quality);
}

}  // namespace cc
