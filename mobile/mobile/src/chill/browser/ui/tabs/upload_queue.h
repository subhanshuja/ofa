// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_UI_TABS_UPLOAD_QUEUE_H_
#define CHILL_BROWSER_UI_TABS_UPLOAD_QUEUE_H_

#include <memory>
#include <set>

namespace opera {
namespace tabui {

class ImageQuad;

// Abstract base class for texture upload queues.
class UploadQueue {
 public:
  virtual ~UploadQueue();

  // Creates an upload queue suitable for the actual gpu.
  static std::unique_ptr<UploadQueue> Create();

  // Enqeue image quad for upload.
  virtual void Enqueue(const std::shared_ptr<ImageQuad>& image_quad);

  // Called when a new frame is drawn.
  virtual void OnDrawFrame() = 0;

 protected:
  // Element representing an image quad with priority.
  class Key {
   public:
    Key();
    Key(const std::shared_ptr<ImageQuad>& image_quad, int priority);

    int priority() const { return priority_; }
    size_t id() const { return id_; }
    const std::weak_ptr<ImageQuad>& image_quad() const { return image_quad_; }

   private:
    int priority_;
    size_t id_;
    std::weak_ptr<ImageQuad> image_quad_;
  };

  // Sort order, i.e. image quads with lowest priority will be served first.
  struct Order {
    bool operator()(Key lhs, Key rhs);
  };

  // Sorted set of image quads with priority.
  std::set<Key, Order> queue_;
};

}  // namespace tabui
}  // namespace opera

#endif  // CHILL_BROWSER_UI_TABS_UPLOAD_QUEUE_H_
