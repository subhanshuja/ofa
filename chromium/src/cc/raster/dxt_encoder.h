// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RESOURCES_DXT_ENCODER_H_
#define CC_RESOURCES_DXT_ENCODER_H_

#include <stdint.h>

#include "cc/raster/texture_compressor.h"

namespace cc {

// ATC compression works on blocks of 4 by 4 texels. Width and height of the
// source image must be multiple of 4.
void CompressATC(const uint8_t* src,
                 uint8_t* dst,
                 int width,
                 int height,
                 bool opaque,
                 TextureCompressor::Quality quality);

// DXT compression works on blocks of 4 by 4 texels. Width and height of the
// source image must be multiple of 4.
void CompressDXT(const uint8_t* src,
                 uint8_t* dst,
                 int width,
                 int height,
                 bool opaque,
                 TextureCompressor::Quality quality);

}  // namespace cc

#endif  // CC_RESOURCES_DXT_ENCODER_H_