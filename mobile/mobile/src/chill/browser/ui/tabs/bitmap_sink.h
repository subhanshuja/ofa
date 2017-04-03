// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_UI_TABS_BITMAP_SINK_H_
#define CHILL_BROWSER_UI_TABS_BITMAP_SINK_H_

#include <jni.h>
#include <memory>

class SkBitmap;

namespace opera {
namespace tabui {

class ThumbnailFeeder;

// Sink that accepts a bitmap. The accepted bitmap is handed over to a
// ThumbnailFeeder for further processing.
//
// The class is implemented as an opaque pointer to the real implementaion,
// allowing it to be passed by value between C++ and Java.
class BitmapSink {
 public:
  // Constructs a new BitmapSink bound to a given tab id. After Accept() or
  // Fail() has been called, the BitmapSink is consumed and must not be used
  // again.
  BitmapSink(const ThumbnailFeeder& thumbnail_feeder, int tab_id);

  // Constructs a new BitmapSink bound to a given tab id. The runnable will be
  // posted using the handler either when Accept() has been called, and the
  // given bitmap has reached the ThumbnailCache, or when Fail() has been
  // called. After the runnable has been posted the BitmapSink is consumed and
  // must not be used again.
  BitmapSink(const ThumbnailFeeder& thumbnail_feeder, int tab_id,
             jobject runnable, jobject handler);

  // Posts the runnable also if no bitmap is available.
  void Fail();

  // Updates thumbnail cache and posts runnable when the new thumbnail is
  // available. The provided bitmap must not be written to after the call.
  void Accept(jobject bitmap);
  void Accept(const SkBitmap& bitmap);

 private:
  class Impl;

  std::shared_ptr<Impl> impl_;
};

}  // namespace tabui
}  // namespace opera

#endif  // CHILL_BROWSER_UI_TABS_BITMAP_SINK_H_
