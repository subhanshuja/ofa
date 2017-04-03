// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_UI_TABS_ASYNCHRONOUS_UPLOAD_QUEUE_H_
#define CHILL_BROWSER_UI_TABS_ASYNCHRONOUS_UPLOAD_QUEUE_H_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

#include <EGL/egl.h>

#include "chill/browser/ui/tabs/upload_queue.h"

namespace opera {
namespace tabui {

// UploadQueue implementation for asynchronous texture uploads.
class AsynchronousUploadQueue : public UploadQueue {
  AsynchronousUploadQueue(const AsynchronousUploadQueue&) = delete;
  AsynchronousUploadQueue operator=(const AsynchronousUploadQueue&) = delete;

 public:
  AsynchronousUploadQueue();
  ~AsynchronousUploadQueue();

  void Enqueue(const std::shared_ptr<ImageQuad>& image_quad) override;

  // Acquire recently uploaded textures. Should be called before a new frame is
  // drawn. Each call brings recently uploaded textures through memory barriers
  // and it will take two calls to bring a texture from being uploaded to be
  // effective in the following draw.
  void OnDrawFrame() override;

  // Returns true if the worker thread failed to start.
  bool failed() const { return failed_; }

 private:
  void WorkerMain(EGLDisplay display, EGLContext renderer_context);

  // Set of image quads waiting for acquire.
  std::unordered_map<size_t, std::weak_ptr<ImageQuad>> acquire_image_quads_;

  // Guards access to internal variables.
  std::mutex mutex_;

  // Set if the worker thread failed to start.
  bool failed_;

  // Set when the worker thread should terminate at next wake-up.
  bool destroyed_;

  // Set when synchronization is requested.
  bool sync_requested_;

  // For wake-up calls to worker thread.
  std::condition_variable cv_;

  // Worker thread.
  std::thread worker_thread_;
};

}  // namespace tabui
}  // namespace opera

#endif  // CHILL_BROWSER_UI_TABS_ASYNCHRONOUS_UPLOAD_QUEUE_H_
