// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/ui/tabs/asynchronous_upload_queue.h"

#include <algorithm>
#include <utility>

#include "base/logging.h"

#include "chill/browser/ui/tabs/image_quad.h"
#include "common/opengl_utils/gl_mipmap.h"
#include "common/opengl_utils/gl_texture.h"

namespace opera {
namespace tabui {

AsynchronousUploadQueue::AsynchronousUploadQueue()
    : failed_(false), destroyed_(false), sync_requested_(false) {
  // Must be on renderer thread to get context.
  CHECK_NE(eglGetCurrentContext(), EGL_NO_CONTEXT);

  std::unique_lock<std::mutex> scoped_lock(mutex_);
  worker_thread_ = std::thread(&AsynchronousUploadQueue::WorkerMain, this,
                               eglGetCurrentDisplay(), eglGetCurrentContext());

  // Wait for worker thread to start.
  cv_.wait(scoped_lock);
}

AsynchronousUploadQueue::~AsynchronousUploadQueue() {
  // Notify worker thread and wait for it to terminate.
  {
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    destroyed_ = true;
    cv_.notify_one();
  }
  worker_thread_.join();
}

void AsynchronousUploadQueue::Enqueue(const std::shared_ptr<ImageQuad>&
                                      image_quad) {
  DCHECK(!failed_);
  std::unique_lock<std::mutex> scoped_lock(mutex_);
  UploadQueue::Enqueue(image_quad);

  // Notify worker thread there is work to do.
  cv_.notify_one();
}

void AsynchronousUploadQueue::OnDrawFrame() {
  std::unordered_map<size_t, std::weak_ptr<ImageQuad>> acquire_image_quads;
  {
    DCHECK(!failed_);
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    swap(acquire_image_quads, acquire_image_quads_);
    sync_requested_ = true;
    cv_.notify_one();
  }

  if (acquire_image_quads.empty())
    return;

  for (auto key : acquire_image_quads) {
    // Try to upgrade weak pointer. It will fail if referred image_quad was
    // deleted.
    std::shared_ptr<ImageQuad> image_quad = key.second.lock();
    if (image_quad) {
      image_quad->Acquire();
    }
  }
}

void AsynchronousUploadQueue::WorkerMain(EGLDisplay display,
                                  EGLContext renderer_context) {
  CHECK_NE(display, EGL_NO_DISPLAY);
  CHECK_NE(renderer_context, EGL_NO_CONTEXT);

  EGLint ret;

  ret = eglInitialize(display, NULL, NULL);
  CHECK_EQ(ret, EGL_TRUE);

  // Config.
  static EGLint const config_attribute_list[] = {
      EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE,
      EGL_OPENGL_ES2_BIT, EGL_NONE};
  EGLConfig config;
  EGLint num_config;
  ret =
      eglChooseConfig(display, config_attribute_list, &config, 1, &num_config);
  CHECK_EQ(ret, EGL_TRUE);
  CHECK_EQ(num_config, 1);

  // Surface.
  static EGLint const surface_attribute_list[] = {EGL_WIDTH, 64, EGL_HEIGHT, 64,
                                                  EGL_NONE};
  EGLSurface surface =
      eglCreatePbufferSurface(display, config, surface_attribute_list);
  CHECK_NE(surface, EGL_NO_SURFACE);

  // Shared context.
  static EGLint const context_attribute_list[] = {EGL_CONTEXT_CLIENT_VERSION, 2,
                                                  EGL_NONE};

  EGLContext context = eglCreateContext(display, config, renderer_context,
                                        context_attribute_list);
  if (context == EGL_NO_CONTEXT) {
    // Asynchronous upload is not possible if we can't create a shared context.
    // Thus make a graceful shutdown of this thread.
    LOG(WARNING) << "Failed to create context";

    // Destroy surface.
    if (eglDestroySurface(display, surface) != EGL_TRUE)
      LOG(ERROR) << "Failed to destroy surface";

    // Terminate display connection.
    if (eglTerminate(display) != EGL_TRUE)
      LOG(ERROR) << "Failed to terminate display connection";

    // Signal main thread that this thread has failed.
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    failed_ = true;
    cv_.notify_one();
    return;
  } else {
    // Signal main thread that this thread has started successfully.
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    cv_.notify_one();
  }

  // Make the new context current.
  ret = eglMakeCurrent(display, surface, surface, context);
  CHECK_EQ(ret, EGL_TRUE);

  // Set of image quads that need to be synced.
  std::unordered_map<size_t, std::weak_ptr<ImageQuad>> release_image_quads;

  // Main loop. Poll queue for work to do and upload using created context.
  while (true) {
    CHECK_NE(eglGetCurrentContext(), EGL_NO_CONTEXT);

    // If synchronization requested.
    bool sync_requested = false;
    {
      std::unique_lock<std::mutex> scoped_lock(mutex_);
      sync_requested = sync_requested_;
      sync_requested_ = false;
    }
    if (sync_requested && !release_image_quads.empty()) {
      GLTexture::Flush();
      for (auto key : release_image_quads) {
        // Try to upgrade weak pointer. It will fail if referred image quad was
        // deleted.
        std::shared_ptr<ImageQuad> image_quad = key.second.lock();
        if (image_quad) {
          image_quad->Release();
        }
      }

      {
        std::unique_lock<std::mutex> scoped_lock(mutex_);
        acquire_image_quads_.insert(release_image_quads.begin(),
                                    release_image_quads.end());
      }
      release_image_quads.clear();
    }

    // Grab first queued job.
    Key key;
    {
      std::unique_lock<std::mutex> scoped_lock(mutex_);
      if (!queue_.empty()) {
        auto it = queue_.begin();
        key = *it;
        queue_.erase(it);
      }
    }

    // Try upgrade weak pointer. It will fail if queue was empty or referred
    // image quad was deleted.
    std::shared_ptr<ImageQuad> image_quad = key.image_quad().lock();
    if (image_quad) {
      DCHECK_EQ(key.id(), image_quad->id());

      image_quad->Upload(true);
      release_image_quads[key.id()] = key.image_quad();
      Enqueue(image_quad);
    }

    // If nothing to do, wait for something to do.
    std::unique_lock<std::mutex> scoped_lock(mutex_);
    if (destroyed_)
      break;

    if (!sync_requested_ && queue_.empty())
      cv_.wait(scoped_lock);
  }

  // Clean up allocated gl resources before terminating.
  ret = eglMakeCurrent(display, 0, 0, 0);
  CHECK_EQ(ret, EGL_TRUE);

  // Destroy context.
  ret = eglDestroyContext(display, context);
  CHECK_EQ(ret, EGL_TRUE);

  // Destroy surface.
  ret = eglDestroySurface(display, surface);
  CHECK_EQ(ret, EGL_TRUE);

  // Terminate display connection.
  ret = eglTerminate(display);
  CHECK_EQ(ret, EGL_TRUE);
}

}  // namespace tabui
}  // namespace opera
