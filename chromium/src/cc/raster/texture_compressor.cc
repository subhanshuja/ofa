// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/raster/texture_compressor.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "cc/raster/texture_compressor_atc.h"
#include "cc/raster/texture_compressor_dxt.h"
#include "cc/raster/texture_compressor_etc1.h"

#if defined(ARCH_CPU_X86_FAMILY)
#include "base/cpu.h"
#include "cc/raster/texture_compressor_etc1_sse.h"
#endif

#if defined(OS_ANDROID) && defined(ARCH_CPU_ARMEL)
#include <cpu-features.h>
#include "cc/raster/texture_compressor_atc_neon.h"
#include "cc/raster/texture_compressor_dxt_neon.h"
#include "cc/raster/texture_compressor_etc1_neon.h"
#endif

namespace cc {

std::unique_ptr<TextureCompressor> TextureCompressor::Create(Format format) {
  switch (format) {
    case kFormatATC:
    case kFormatATCIA:
#if defined(OS_ANDROID) && defined(ARCH_CPU_ARMEL)
      if ((android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) != 0) {
        return base::WrapUnique(new TextureCompressorATC_NEON(
            format == kFormatATCIA));
      }
#endif
      return base::WrapUnique(new TextureCompressorATC(
          format == kFormatATCIA));
    case kFormatDXT1:
    case kFormatDXT5:
#if defined(OS_ANDROID) && defined(ARCH_CPU_ARMEL)
      if ((android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) != 0) {
        return base::WrapUnique(new TextureCompressorDXT_NEON(
            format == kFormatDXT5));
      }
#endif
      return base::WrapUnique(new TextureCompressorDXT(
          format == kFormatDXT5));
    case kFormatETC1: {
#if defined(ARCH_CPU_X86_FAMILY)
      base::CPU cpu;
      if (cpu.has_sse2()) {
        return base::WrapUnique(new TextureCompressorETC1SSE());
      }
#endif
#if defined(OS_ANDROID) && defined(ARCH_CPU_ARMEL)
      if ((android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) != 0) {
        return base::WrapUnique(new TextureCompressorETC1_NEON());
      }
#endif
      return base::WrapUnique(new TextureCompressorETC1());
    }
  }

  NOTREACHED();
  return nullptr;
}

}  // namespace cc
