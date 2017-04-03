// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_SKIA_SKIA_UTILS_H_
#define CHILL_BROWSER_SKIA_SKIA_UTILS_H_

#include <android/bitmap.h>

#include "base/android/jni_android.h"

class SkBitmap;
class SkData;

namespace opera {

class SkiaUtils {
 public:
  static void CopyToJavaBitmap(SkBitmap* src_bitmap, jobject dst_bitmap);
  static bool CopyFromJavaBitmap(SkBitmap* bitmap, jobject src_bitmap);
  static SkBitmap* CreateFromJavaBitmap(jobject src_bitmap);
  static SkBitmap* Decode(SkData* data);
  static AndroidBitmapFormat GetFormat(SkBitmap* bitmap);
  static bool Scale(SkBitmap* bitmap, jobject dst_bitmap);
  static bool DrawScaled(SkBitmap* bitmap,
                         jobject dst_bitmap,
                         int width,
                         int height);
  static SkBitmap* Crop(SkBitmap* bitmap, int x, int y, int width, int height);
  static int AverageColor(
      SkBitmap* bitmap, int x, int y, int width, int height);
  static int HistogramColor(SkBitmap* bitmap, int x, int y, int width,
      int height);
};

}  // namespace opera

#endif  // CHILL_BROWSER_SKIA_SKIA_UTILS_H_
