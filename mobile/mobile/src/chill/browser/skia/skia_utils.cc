// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/skia/skia_utils.h"

#include <string.h>
#include <android/bitmap.h>

#include <algorithm>

#include "base/logging.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkStream.h"
#include "ui/gfx/android/java_bitmap.h"
#include "ui/gfx/codec/jpeg_codec.h"

#include "common/bitmap/scale.h"

namespace opera {

static AndroidBitmapFormat ToAndroidBitmapFormat(SkColorType type) {
  switch (type) {
    case SkColorType::kUnknown_SkColorType:
      return ANDROID_BITMAP_FORMAT_NONE;
    case SkColorType::kAlpha_8_SkColorType:
      return ANDROID_BITMAP_FORMAT_A_8;
    case SkColorType::kRGB_565_SkColorType:
      return ANDROID_BITMAP_FORMAT_RGB_565;
    case SkColorType::kARGB_4444_SkColorType:
      return ANDROID_BITMAP_FORMAT_RGBA_4444;
    case SkColorType::kRGBA_8888_SkColorType:
      return ANDROID_BITMAP_FORMAT_RGBA_8888;
    default: {
      NOTREACHED() << "Unsupported format";
      return ANDROID_BITMAP_FORMAT_NONE;
    }
  }
}

static SkColorType ToSkColorType(AndroidBitmapFormat format) {
  switch (format) {
    case ANDROID_BITMAP_FORMAT_NONE:
      return SkColorType::kUnknown_SkColorType;
    case ANDROID_BITMAP_FORMAT_RGBA_8888:
      return SkColorType::kRGBA_8888_SkColorType;
    case ANDROID_BITMAP_FORMAT_RGB_565:
      return SkColorType::kRGB_565_SkColorType;
    case ANDROID_BITMAP_FORMAT_RGBA_4444:
      return SkColorType::kARGB_4444_SkColorType;
    case ANDROID_BITMAP_FORMAT_A_8:
      return SkColorType::kAlpha_8_SkColorType;
    default:
      NOTREACHED() << "Unsupported format";
      return SkColorType::kUnknown_SkColorType;
  }
}

namespace {

class ScaleSkBitmap {
 public:
  explicit ScaleSkBitmap(SkBitmap* bitmap) : bitmap_(bitmap) {}
  ScaleSkBitmap* CreateCopy(int width, int height) const {
    ScaleSkBitmap* ret = new ScaleSkBitmap(new SkBitmap());
    ret->own_bitmap_.reset(ret->bitmap_);
    ret->bitmap_->setInfo(
        SkImageInfo::Make(
            width, height, bitmap_->colorType(), kPremul_SkAlphaType), 0);
    if (!ret->bitmap_->tryAllocPixels()) {
      delete ret;
      return NULL;
    }
    return ret;
  }
  int width() const {
    return bitmap_->width();
  }
  int height() const {
    return bitmap_->height();
  }
  AndroidBitmapFormat format() const {
    return ToAndroidBitmapFormat(bitmap_->colorType());
  }
  int stride() const {
    return bitmap_->rowBytes();
  }
  void LockPixels() {
    bitmap_->lockPixels();
  }
  void UnlockPixels() {
    bitmap_->unlockPixels();
  }
  void* GetRow(int y) {
    return bitmap_->getAddr(0, y);
  }
  bool empty() const {
    return bitmap_->empty();
  }

 private:
  std::unique_ptr<SkBitmap> own_bitmap_;
  SkBitmap* bitmap_;
};

class ScaleJavaBitmap {
 public:
  explicit ScaleJavaBitmap(jobject bitmap) : bitmap_(bitmap) {}
  int width() const {
    return bitmap_.size().width();
  }
  int height() const {
    return bitmap_.size().height();
  }
  AndroidBitmapFormat format() const {
    return (AndroidBitmapFormat) bitmap_.format();
  }
  int stride() const {
    return bitmap_.stride();
  }
  void LockPixels() {
  }
  void UnlockPixels() {
  }
  void* GetRow(int y) {
    return static_cast<char*>(bitmap_.pixels()) + bitmap_.stride() * y;
  }
  bool empty() const {
    return width() < 1 || height() < 1;
  }

 private:
  gfx::JavaBitmap bitmap_;
};

}  // namespace

void SkiaUtils::CopyToJavaBitmap(SkBitmap* src_bitmap, jobject dst_bitmap) {
  gfx::JavaBitmap dst_lock(dst_bitmap);
  SkAutoLockPixels src_lock(*src_bitmap);

  DCHECK_EQ(src_bitmap->colorType(), ToSkColorType(
      static_cast<AndroidBitmapFormat>(dst_lock.format())));
  DCHECK_EQ(dst_lock.size().width(), src_bitmap->width());
  DCHECK_EQ(dst_lock.size().height(), src_bitmap->height());

  uint32_t bytes_per_row = src_bitmap->width() *
      BytesPerPixel((AndroidBitmapFormat) dst_lock.format());
  uint8_t* dst_pixels = static_cast<uint8_t*>(dst_lock.pixels());
  for (int k = 0; k < src_bitmap->height(); ++k) {
    memcpy(dst_pixels, src_bitmap->getAddr(0, k), bytes_per_row);
    dst_pixels += dst_lock.stride();
  }

  return;
}

bool SkiaUtils::CopyFromJavaBitmap(SkBitmap* bitmap, jobject src_bitmap) {
  DCHECK(bitmap);

  gfx::JavaBitmap src_lock(src_bitmap);

  int bmW = src_lock.size().width(), bmH = src_lock.size().height();
  SkColorType ct = ToSkColorType(
      static_cast<AndroidBitmapFormat>(src_lock.format()));
  bitmap->setInfo(SkImageInfo::Make(bmW, bmH, ct, kPremul_SkAlphaType), 0);
  if (!bitmap->tryAllocPixels()) {
    return false;
  }

  SkAutoLockPixels dst_lock(*bitmap);
  uint32_t bytes_per_row = src_lock.size().width() *
      BytesPerPixel((AndroidBitmapFormat) src_lock.format());
  uint8_t* src_pixels = static_cast<uint8_t*>(src_lock.pixels());
  for (int k = 0; k < src_lock.size().height(); ++k) {
    memcpy(bitmap->getAddr(0, k), src_pixels, bytes_per_row);
    src_pixels += src_lock.stride();
  }
  return true;
}

SkBitmap* SkiaUtils::CreateFromJavaBitmap(jobject src_bitmap) {
  std::unique_ptr<SkBitmap> copy(new SkBitmap());
  return CopyFromJavaBitmap(copy.get(), src_bitmap) ? copy.release() : NULL;
}

SkBitmap* SkiaUtils::Decode(SkData* data) {
  return gfx::JPEGCodec::Decode(
             reinterpret_cast<const unsigned char*>(data->data()), data->size())
      .release();
}

AndroidBitmapFormat SkiaUtils::GetFormat(SkBitmap* bitmap) {
  return ToAndroidBitmapFormat(bitmap->colorType());
}

bool SkiaUtils::Scale(SkBitmap* bitmap, jobject dst_bitmap) {
  int width, height;
  {
    gfx::JavaBitmap dst_lock(dst_bitmap);
    width = dst_lock.size().width();
    height = dst_lock.size().height();
  }
  return DrawScaled(bitmap, dst_bitmap, width, height);
}

bool SkiaUtils::DrawScaled(SkBitmap* bitmap,
                           jobject dst_bitmap,
                           int width,
                           int height) {
  ScaleJavaBitmap dst(dst_bitmap);
  ScaleSkBitmap src(bitmap);
  return ScalePixels(dst, src, width, height);
}

SkBitmap* SkiaUtils::Crop(SkBitmap* bitmap,
                          int x,
                          int y,
                          int width,
                          int height) {
  SkBitmap dst;
  if (!bitmap->extractSubset(&dst, SkIRect::MakeXYWH(x, y, width, height))) {
    return NULL;
  }

  return new SkBitmap(dst);
}

int SkiaUtils::AverageColor(SkBitmap* bitmap,
                            int x,
                            int y,
                            int width,
                            int height) {
  static const int default_color = 0xffffffff;

  if (!bitmap || bitmap->empty()) {
    return default_color;
  }

  // Clamp rect to bitmap boundaries.
  SkIRect rect;
  bitmap->getBounds(&rect);
  if (!rect.intersect(SkIRect::MakeXYWH(x, y, width, height))) {
    return default_color;
  }

  // Set up format dependent scan line operations.
  void (*ColorSum)(void*, int, Color128*);
  switch (bitmap->colorType()) {
    case SkColorType::kRGB_565_SkColorType: {
      ColorSum = ColorSum_RGB565;
      break;
    }
    case SkColorType::kRGBA_8888_SkColorType: {
      ColorSum = ColorSum_ARGB8888;
      break;
    }
    default: {
      NOTIMPLEMENTED() << "Unsupported format";
      return default_color;
    }
  }

  // Calculate sum of all pixels in the area.
  Color128 sum;
  sum.a = sum.r = sum.g = sum.b = 0;
  SkAutoLockPixels lock(*bitmap);
  for (int row = rect.top(); row < rect.bottom(); row++) {
    void* pixels = bitmap->getAddr(rect.left(), row);
    ColorSum(pixels, rect.width(), &sum);
  }

  // Calculate average color (divide sum by number of pixels).
  int num_pixels = rect.width() * rect.height();
  int rounding = num_pixels / 2;
  sum.a = (sum.a + rounding) / num_pixels;
  sum.r = (sum.r + rounding) / num_pixels;
  sum.g = (sum.g + rounding) / num_pixels;
  sum.b = (sum.b + rounding) / num_pixels;
  DCHECK(sum.a <= 255 && sum.r <= 255 && sum.g <= 255 && sum.b <= 255);

  // Return color in Android Paint format.
  return (sum.a << 24) | (sum.r << 16) | (sum.g << 8) | sum.b;
}

int SkiaUtils::HistogramColor(SkBitmap* bitmap,
                              int x,
                              int y,
                              int width,
                              int height) {
  static const int default_color = 0xffffffff;

  if (!bitmap || bitmap->empty()) {
    return default_color;
  }

  // Clamp rect to bitmap boundaries.
  SkIRect rect;
  bitmap->getBounds(&rect);
  if (!rect.intersect(SkIRect::MakeXYWH(x, y, width, height))) {
    return default_color;
  }

  // Set up format dependent scan line operations.
  void (*ColorHist)(void*, int, int*, int*, int*);
  switch (bitmap->colorType()) {
    case SkColorType::kRGB_565_SkColorType: {
      ColorHist = ColorHist_RGB565;
      break;
    }
    case SkColorType::kRGBA_8888_SkColorType: {
      ColorHist = ColorHist_ARGB8888;
      break;
    }
    default: {
      NOTIMPLEMENTED() << "Unsupported format";
      return default_color;
    }
  }

  // Calculate R, G and B histograms of all pixels in the area.
  int hist_r[256], hist_g[256], hist_b[256];
  memset(hist_r, 0, 256 * sizeof(int));
  memset(hist_g, 0, 256 * sizeof(int));
  memset(hist_b, 0, 256 * sizeof(int));
  SkAutoLockPixels lock(*bitmap);
  for (int row = rect.top(); row < rect.bottom(); row++) {
    void* pixels = bitmap->getAddr(rect.left(), row);
    ColorHist(pixels, rect.width(), hist_r, hist_g, hist_b);
  }

  // Find most common R, G and B values.
  int r = std::distance(hist_r, std::max_element(hist_r, hist_r + 256));
  int r_max = hist_r[r];
  int g = std::distance(hist_g, std::max_element(hist_g, hist_g + 256));
  int g_max = hist_g[g];
  int b = std::distance(hist_b, std::max_element(hist_b, hist_b + 256));
  int b_max = hist_b[b];

  // Check if the histogram results are reliable, i.e:
  // 1) Do we have at least 10% of the total number of pixels.
  // 2) Do we have roughly the same results for R, G and B (max 10% diff between
  //    min and max).
  int total_hist = rect.width() * rect.height();
  int max_hist = std::max(r_max, std::max(g_max, b_max));
  int min_hist = std::min(r_max, std::min(g_max, b_max));
  DCHECK(total_hist > 0 && min_hist > 0);
  if ((min_hist < total_hist / 10) || ((100 * max_hist) / min_hist > 110)) {
    // Calculate average color from the histograms.
    r = g = b = 0;
    for (int i = 0; i < 256; ++i) {
      r += hist_r[i] * i;
      g += hist_g[i] * i;
      b += hist_b[i] * i;
    }
    r /= total_hist;
    g /= total_hist;
    b /= total_hist;

    // Fall back to black for dark images, and white for light images.
    const int white_threshold = 100;
    if (r + g + b >= white_threshold * 3) {
      r = g = b = 255;
    } else {
      r = g = b = 0;
    }
  }

  // Return combined color in Android Paint format.
  return (255 << 24) | (r << 16) | (g << 8) | b;
}

}  // namespace opera
