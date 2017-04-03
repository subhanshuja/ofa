// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_BITMAP_SCALE_H_
#define COMMON_BITMAP_SCALE_H_

#include <android/bitmap.h>

#include <algorithm>

#include "base/logging.h"

#include "common/bitmap/op_scanline_ops.h"

namespace {

int BytesPerPixel(AndroidBitmapFormat format) {
  switch (format) {
    case ANDROID_BITMAP_FORMAT_NONE:
      break;
    case ANDROID_BITMAP_FORMAT_RGBA_8888:
      return 4;
    case ANDROID_BITMAP_FORMAT_RGB_565:
    case ANDROID_BITMAP_FORMAT_RGBA_4444:
      return 2;
    case ANDROID_BITMAP_FORMAT_A_8:
      return 1;
  }
  return 0;
}

template<typename DstBitmap, typename SrcBitmap>
bool ScalePixels(DstBitmap& dst, SrcBitmap& src, int width, int height) {
  std::unique_ptr<SrcBitmap> tmp_bitmap;

  if (width < 1 || height < 1 || src.empty()) {
    return false;
  }

  // Check destination rect against destination bitmap dimensions (we currently
  // don't implement automatic cropping).
  if (width > dst.width() || height > dst.height()) {
    return false;
  }

  DCHECK_EQ(dst.format(), src.format());

  // Set up format dependent scan line operations.
  void (*HalveXY)(void*, void*, void*, int);
  void (*LerpX)(void*, void*, int, float);
  void (*LerpY)(void*, void*, void*, int, float);
  switch (src.format()) {
    case ANDROID_BITMAP_FORMAT_RGB_565:
      HalveXY = HalveXY_RGB565;
      LerpX = LerpX_RGB565;
      LerpY = LerpY_RGB565;
      break;
    case ANDROID_BITMAP_FORMAT_RGBA_8888:
      HalveXY = HalveXY_ARGB8888;
      LerpX = LerpX_ARGB8888;
      LerpY = LerpY_ARGB8888;
      break;
    default: {
      NOTIMPLEMENTED() << "Unsupported format";
      return false;
    }
  }

  // Step1: Iterative downscale with box-filter (eliminate aliasing effects and
  // make the bilinear interpolation step faster).
  // Note: This is only done for down-scaling operations.
  SrcBitmap* src_bitmap = &src;
  int bmW = src.width(), bmH = src.height();
  while (bmW >= (width * 2)) {
    bmW /= 2;
    bmH /= 2;
    std::unique_ptr<SrcBitmap> tmp(src_bitmap->CreateCopy(bmW, bmH));
    if (!tmp)
      return false;
    tmp->LockPixels();
    src_bitmap->LockPixels();
    for (int k = 0; k < bmH; ++k) {
      void* dst = tmp->GetRow(k);
      void* src1 = src_bitmap->GetRow(2 * k);
      void* src2 = src_bitmap->GetRow(2 * k + 1);
      HalveXY(dst, src1, src2, bmW);
    }
    tmp->UnlockPixels();
    src_bitmap->UnlockPixels();
    tmp_bitmap = std::move(tmp);
    src_bitmap = tmp_bitmap.get();
  }

  // If the bitmap was scaled down to zero, use the original bitmap for
  // interpolation. This has the drawback of more aliasing, but it should very
  // rarely happen.
  if (src_bitmap->empty()) {
    src_bitmap = &src;
    tmp_bitmap.reset();
  }

  // Step 2: Blit bitmap to destination, using scaling if necessary.
  src_bitmap->LockPixels();
  dst.LockPixels();

  if (width == src_bitmap->width() && height == src_bitmap->height()) {
    // We can use memcpy for blitting.
    if (width == dst.width() && src_bitmap->stride() == dst.stride()) {
      memcpy(dst.GetRow(0), src_bitmap->GetRow(0), dst.stride() * height);
    } else {
      uint32_t bytes_per_pixel = BytesPerPixel(dst.format());
      for (int k = 0; k < height; ++k) {
        memcpy(dst.GetRow(k), src_bitmap->GetRow(k), width * bytes_per_pixel);
      }
    }
  } else {
    // Allocate temporary rows for X-lerping.
    std::unique_ptr<uint8_t[]> lines[2];
    lines[0].reset(new uint8_t[width * BytesPerPixel(dst.format())]);
    lines[1].reset(new uint8_t[width * BytesPerPixel(dst.format())]);
    if (!lines[0].get() || !lines[1].get()) {
      src_bitmap->UnlockPixels();
      dst.UnlockPixels();
      return false;
    }
    int line_no = -2;

    // Initialize scaling operation.
    float x_step = 0.0f, y_step = 0.0f, src_y = 0.0f;
    {
      if (width > 1) {
        x_step = static_cast<float>(src_bitmap->width() - 1) /
            static_cast<float>(width - 1);
      }
      if (height > 1) {
        y_step = static_cast<float>(src_bitmap->height() - 1) /
            static_cast<float>(height - 1);
      }
    }

    // Note: At this point, src_bitmap->height() >= 1.
    int max_src_y = src_bitmap->height() - 1;
    for (int k = 0; k < height; ++k) {
      // Lerp along X if necessary. Here we try to reuse lines when possible.
      int sy = static_cast<int>(src_y);
      int sy1 = std::min(sy, max_src_y);
      int sy2 = std::min(sy + 1, max_src_y);
      if (line_no == sy1) {
        // Turn the bottom row into the top row, and generate bottom row.
        lines[0].swap(lines[1]);
        LerpX(lines[1].get(), src_bitmap->GetRow(sy2), width, x_step);
      } else if (line_no < sy1) {
        // We need to generate both the bottom and the top row.
        LerpX(lines[0].get(), src_bitmap->GetRow(sy1), width, x_step);
        LerpX(lines[1].get(), src_bitmap->GetRow(sy2), width, x_step);
      }
      line_no = sy2;

      // Lerp along Y.
      LerpY(dst.GetRow(k), lines[0].get(), lines[1].get(), width, src_y);

      src_y += y_step;
    }
  }

  src_bitmap->UnlockPixels();
  dst.UnlockPixels();
  return true;
}

}  // namespace

#endif  // COMMON_BITMAP_SCALE_H_
