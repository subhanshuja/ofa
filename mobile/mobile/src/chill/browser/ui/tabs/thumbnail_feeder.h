// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_UI_TABS_THUMBNAIL_FEEDER_H_
#define CHILL_BROWSER_UI_TABS_THUMBNAIL_FEEDER_H_

#include <jni.h>
#include <memory>

class SkBitmap;

namespace opera {
namespace tabui {

class BitmapSink;
class ThumbnailCache;

// ThumbnailFeeder feeds the cache with bitmaps. The conversion from bitmap to
// GLMipMap, which includes construction of compressed mipmaps, is performed
// asynchronously on a dedicated worker thread to offload the main thread.
//
// The class is implemented as an opaque pointer to the real implementaion,
// allowing it to be passed by value between C++ and Java.
class ThumbnailFeeder {
 public:
  struct Notifier {
    virtual ~Notifier() {}
  };

  explicit ThumbnailFeeder(const ThumbnailCache& cache);

  // Updates ThumbnailCache given a tab id and a Java Bitmap. The bitmap will be
  // consumed in a different thread and the caller is responsible for not
  // mutating shared pixel data after the bitmap has been handed over.
  // The provided notifier is deleted when the new bitmap has landed in cache.
  void UpdateBitmap(int tab_id,
                    jobject bitmap,
                    std::unique_ptr<Notifier> notifier);

  // Updates ThumbnailCache given a tab id and an SkBitmap. The bitmap will be
  // consumed in a different thread and the caller is responsible for not
  // mutating shared pixel data after the bitmap has been handed over.
  // The provided notifier is deleted when the new bitmap has landed in cache.
  void UpdateBitmap(int tab_id,
                    const SkBitmap& bitmap,
                    std::unique_ptr<Notifier> notifier);

  // Constructs a BitmapSink that will feed the consumed bitmap into this
  // ThumbnailFeeder. The parameters are passed to the BitmapSink constructor.
  BitmapSink* CreateBitmapSink(int tab_id);
  BitmapSink* CreateBitmapSink(int tab_id, jobject runnable, jobject handler);

 private:
  class Impl;

  std::shared_ptr<Impl> impl_;
};

}  // namespace tabui
}  // namespace opera

#endif  // CHILL_BROWSER_UI_TABS_THUMBNAIL_FEEDER_H_
