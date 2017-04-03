// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/ui/tabs/synchronous_upload_queue.h"

#include <chrono>

#include "base/logging.h"

#include "chill/browser/ui/tabs/image_quad.h"

namespace opera {
namespace tabui {

SynchronousUploadQueue::SynchronousUploadQueue() {}

void SynchronousUploadQueue::OnDrawFrame() {
  // Don't start new uploads after 10 ms has passed.
  const std::chrono::milliseconds kMaxDuration { 10 };
  auto stop_time = std::chrono::high_resolution_clock::now() + kMaxDuration;

  while (!queue_.empty() &&
         std::chrono::high_resolution_clock::now() < stop_time) {
    auto it = queue_.begin();

    // Try upgrade weak pointer. Will fail if referred image quad was deleted.
    std::shared_ptr<ImageQuad> image_quad = it->image_quad().lock();
    if (image_quad) {
      DCHECK_EQ(it->id(), image_quad->id());

      image_quad->Upload(false);
    }

    queue_.erase(it);

    // Enqueue upload of next mip level.
    if (image_quad)
      Enqueue(image_quad);
  }
}

}  // namespace tabui
}  // namespace opera
