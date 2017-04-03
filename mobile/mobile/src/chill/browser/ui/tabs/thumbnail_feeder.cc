// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/ui/tabs/thumbnail_feeder.h"

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>

#include "base/android/jni_android.h"
#include "base/android/scoped_java_ref.h"
#include "base/logging.h"
#include "third_party/skia/include/core/SkBitmap.h"

#include "chill/browser/ui/tabs/bitmap_sink.h"
#include "chill/browser/ui/tabs/thumbnail_cache.h"
#include "common/opengl_utils/gl_mipmap.h"

namespace opera {
namespace tabui {

class ThumbnailFeeder::Impl {
 public:
  explicit Impl(const ThumbnailCache& cache)
      : cache_(cache),
        destroyed_(false),
        worker_thread_(&Impl::WorkerMain, this) {}

  ~Impl() {
    // Notify worker thread and wait for it to terminate.
    {
      std::unique_lock<std::mutex> scoped_lock(mutex_);
      destroyed_ = true;
      cv_.notify_one();
    }
    worker_thread_.join();
  }

  void Enqueue(int tab_id, jobject bitmap, std::unique_ptr<Notifier> notifier) {
    Enqueue(std::make_shared<JavaBitmapUpdater>(tab_id, bitmap,
                                                std::move(notifier)));
  }

  void Enqueue(int tab_id,
               const SkBitmap& bitmap,
               std::unique_ptr<Notifier> notifier) {
    Enqueue(
        std::make_shared<SkBitmapUpdater>(tab_id, bitmap, std::move(notifier)));
  }

 private:
  Impl(const Impl&) = delete;
  Impl& operator=(const Impl&) = delete;

  // Base class that stores a tab id and takes ownership of a Notifier.
  class Updater {
   public:
    virtual ~Updater() {
      DCHECK(!notifier_) << "Updater was deleted before Update() was called.";
    }

    // Performs conversion from a bitmap to a GLMipmap on the caller's thread
    // and inserts the result into the given ThumnailCache. The Notifier is
    // relased after the first call. The method can be called several times
    // to insert new GLMipmaps for the same tab id, but not calling it at all
    // is an error.
    virtual void Update(ThumbnailCache* cache) = 0;

   protected:
    Updater(int tab_id, std::unique_ptr<Notifier> notifier)
        : tab_id_(tab_id), notifier_(std::move(notifier)) {
      DCHECK(notifier_);
    }

    template <class Bitmap>
    void Update(ThumbnailCache* cache, const Bitmap& bitmap) {
      // Create a mipmap using the fast strategy.
      std::shared_ptr<GLMipMap> mipmap(
          GLMipMap::Create(bitmap, false, GLMipMap::kCompressIfFast));
      cache->UpdateMipmap(tab_id_, mipmap);
      // Notify a mipmap is ready in the cache.
      notifier_.reset();
      // Update the cache with a compressed mipmap if the fast strategy produced
      // an uncompressed one.
      if (!mipmap->IsCompressed())
        cache->UpdateMipmap(
            tab_id_,
            GLMipMap::Create(bitmap, false, GLMipMap::kCompressAlways));
    }

   private:
    const int tab_id_;
    std::unique_ptr<Notifier> notifier_;
  };

  class JavaBitmapUpdater : public Updater {
   public:
    JavaBitmapUpdater(int tab_id,
                      jobject bitmap,
                      std::unique_ptr<Notifier> notifier)
        : Updater(tab_id, std::move(notifier)),
          bitmap_(base::android::AttachCurrentThread(), bitmap) {}

    void Update(ThumbnailCache* cache) override {
      Updater::Update(cache, bitmap_.obj());
    }

   private:
    base::android::ScopedJavaGlobalRef<jobject> bitmap_;
  };

  class SkBitmapUpdater : public Updater {
   public:
    SkBitmapUpdater(int tab_id,
                    const SkBitmap& bitmap,
                    std::unique_ptr<Notifier> notifier)
        : Updater(tab_id, std::move(notifier)), bitmap_(bitmap) {
      bitmap_.setImmutable();
    }

    void Update(ThumbnailCache* cache) override {
      Updater::Update(cache, bitmap_);
    }

   private:
    SkBitmap bitmap_;
  };

  void Enqueue(const std::shared_ptr<Updater>& updater) {
    // Add last in queue and ping worker thread.
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    queue_.push(updater);
    cv_.notify_one();
  }

  void WorkerMain() {
    base::android::AttachCurrentThreadWithName("ThumbnailFeeder");

    while (true) {
      // Grab all queued jobs.
      std::queue<std::shared_ptr<Updater>> queue;
      {
        std::unique_lock<std::mutex> scoped_lock(mutex_);
        swap(queue, queue_);
      }

      // Run the jobs.
      while (!queue.empty()) {
        queue.front()->Update(&cache_);
        queue.pop();
      }

      // Wait for something to do.
      {
        std::unique_lock<std::mutex> scoped_lock(mutex_);
        if (destroyed_)
          break;

        if (queue_.empty())
          cv_.wait(scoped_lock);
      }
    }

    base::android::DetachFromVM();
  }

  // Cache that should be updated with new images.
  ThumbnailCache cache_;

  // The job queue.
  std::queue<std::shared_ptr<Updater>> queue_;

  // Guards access to internal variables.
  std::mutex mutex_;

  // Set when the worker thread should terminate at next wake-up.
  bool destroyed_;

  // For wake-up calls to worker thread.
  std::condition_variable cv_;

  // Worker thread.
  std::thread worker_thread_;
};

ThumbnailFeeder::ThumbnailFeeder(const ThumbnailCache& cache)
    : impl_(std::make_shared<Impl>(cache)) {}

void ThumbnailFeeder::UpdateBitmap(int tab_id,
                                   jobject bitmap,
                                   std::unique_ptr<Notifier> notifier) {
  impl_->Enqueue(tab_id, bitmap, std::move(notifier));
}

void ThumbnailFeeder::UpdateBitmap(int tab_id,
                                   const SkBitmap& bitmap,
                                   std::unique_ptr<Notifier> notifier) {
  impl_->Enqueue(tab_id, bitmap, std::move(notifier));
}

BitmapSink* ThumbnailFeeder::CreateBitmapSink(int tab_id) {
  return new BitmapSink(*this, tab_id);
}

BitmapSink* ThumbnailFeeder::CreateBitmapSink(int tab_id,
                                              jobject runnable,
                                              jobject handler) {
  return new BitmapSink(*this, tab_id, runnable, handler);
}

}  // namespace tabui
}  // namespace opera
