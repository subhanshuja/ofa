// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_UI_TABS_SYNCHRONOUS_UPLOAD_QUEUE_H_
#define CHILL_BROWSER_UI_TABS_SYNCHRONOUS_UPLOAD_QUEUE_H_

#include "chill/browser/ui/tabs/upload_queue.h"

namespace opera {
namespace tabui {

// UploadQueue implementation for synchronous texture uploads.
class SynchronousUploadQueue : public UploadQueue {
  SynchronousUploadQueue(const SynchronousUploadQueue&) = delete;
  SynchronousUploadQueue operator=(const SynchronousUploadQueue&) = delete;

 public:
  SynchronousUploadQueue();

  void OnDrawFrame() override;
};

}  // namespace tabui
}  // namespace opera

#endif  // CHILL_BROWSER_UI_TABS_SYNCHRONOUS_UPLOAD_QUEUE_H_
