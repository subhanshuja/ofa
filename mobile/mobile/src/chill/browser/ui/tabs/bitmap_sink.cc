// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/ui/tabs/bitmap_sink.h"

#include <utility>

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/logging.h"

#include "chill/browser/ui/tabs/thumbnail_feeder.h"

namespace opera {
namespace tabui {

class BitmapSink::Impl {
 public:
  Impl(const ThumbnailFeeder& thumbnail_feeder, int tab_id)
      : thumbnail_feeder_(thumbnail_feeder),
        tab_id_(tab_id),
        notifier_(new NullNotifier()) {}

  Impl(const ThumbnailFeeder& thumbnail_feeder,
       int tab_id,
       jobject runnable,
       jobject handler)
      : thumbnail_feeder_(thumbnail_feeder),
        tab_id_(tab_id),
        notifier_(new RunnableNotifier(runnable, handler)) {}

  void Fail() {
    DCHECK(notifier_);
    notifier_.reset();
  }

  void Accept(jobject bitmap) {
    DCHECK(notifier_);
    thumbnail_feeder_.UpdateBitmap(tab_id_, bitmap, std::move(notifier_));
  }

  void Accept(const SkBitmap& bitmap) {
    DCHECK(notifier_);
    thumbnail_feeder_.UpdateBitmap(tab_id_, bitmap, std::move(notifier_));
  }

 private:
  // A  notifier that will do nothing.
  class NullNotifier : public ThumbnailFeeder::Notifier {};

  // A notifier that will call a runnable using a handler.
  class RunnableNotifier : public ThumbnailFeeder::Notifier {
   public:
    explicit RunnableNotifier(jobject runnable, jobject handler)
        : runnable_(base::android::AttachCurrentThread(), runnable),
          handler_(base::android::AttachCurrentThread(), handler) {}

    ~RunnableNotifier() override {
      if (runnable_.obj()) {
        // handler.post(callback)
        JNIEnv* env = base::android::AttachCurrentThread();
        jclass cls = env->GetObjectClass(handler_.obj());
        jmethodID mid =
            env->GetMethodID(cls, "post", "(Ljava/lang/Runnable;)Z");
        DCHECK(mid);
        env->CallBooleanMethod(handler_.obj(), mid, runnable_.obj());
      }
    }

   private:
    base::android::ScopedJavaGlobalRef<jobject> runnable_;
    base::android::ScopedJavaGlobalRef<jobject> handler_;
  };

  ThumbnailFeeder thumbnail_feeder_;
  int tab_id_;
  std::unique_ptr<ThumbnailFeeder::Notifier> notifier_;
};

BitmapSink::BitmapSink(const ThumbnailFeeder& thumbnail_feeder, int tab_id)
    : impl_(new Impl(thumbnail_feeder, tab_id)) {}

BitmapSink::BitmapSink(const ThumbnailFeeder& thumbnail_feeder,  int tab_id,
                       jobject runnable, jobject handler)
    : impl_(new Impl(thumbnail_feeder, tab_id, runnable, handler)) {}

void BitmapSink::Fail() {
  impl_->Fail();
}

void BitmapSink::Accept(jobject bitmap) {
  impl_->Accept(bitmap);
}

void BitmapSink::Accept(const SkBitmap& bitmap) {
  impl_->Accept(bitmap);
}

}  // namespace tabui
}  // namespace opera
