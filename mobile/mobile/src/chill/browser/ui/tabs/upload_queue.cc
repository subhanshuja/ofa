// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/ui/tabs/upload_queue.h"

#include "base/logging.h"

#include "chill/browser/ui/tabs/image_quad.h"
#include "chill/browser/ui/tabs/asynchronous_upload_queue.h"
#include "chill/browser/ui/tabs/synchronous_upload_queue.h"
#include "common/opengl_utils/gl_library.h"

namespace opera {
namespace tabui {

UploadQueue::~UploadQueue() {}

std::unique_ptr<UploadQueue> UploadQueue::Create() {
  GLLibrary& gl = GLLibrary::Get();

  if (gl.Init() && gl.SupportsAsyncUpload()) {
    std::unique_ptr<AsynchronousUploadQueue> asynchronous_queue(
        new AsynchronousUploadQueue());
    if (!asynchronous_queue->failed())
      return std::move(asynchronous_queue);
  }

  return std::unique_ptr<UploadQueue>(new SynchronousUploadQueue());
}

void UploadQueue::Enqueue(const std::shared_ptr<ImageQuad>& image_quad) {
  DCHECK(image_quad);
  DCHECK(image_quad->mipmap());

  int priority = image_quad->upload_priority();
  if (priority == std::numeric_limits<int>::min())
    return;

  queue_.emplace(image_quad, priority);
}

UploadQueue::Key::Key() {}
UploadQueue::Key::Key(const std::shared_ptr<ImageQuad>& image_quad,
                      int priority)
    : priority_(priority), id_(image_quad->id()), image_quad_(image_quad) {}

bool UploadQueue::Order::operator()(Key lhs, Key rhs) {
  if (lhs.priority() != rhs.priority())
    return lhs.priority() < rhs.priority();
  return lhs.id() < rhs.id();
}

}  // namespace tabui
}  // namespace opera
