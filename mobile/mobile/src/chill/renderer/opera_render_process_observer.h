// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/shell_render_process_observer.h
// @final-synchronized

#ifndef CHILL_RENDERER_OPERA_RENDER_PROCESS_OBSERVER_H_
#define CHILL_RENDERER_OPERA_RENDER_PROCESS_OBSERVER_H_

#include "base/compiler_specific.h"
#include "content/public/renderer/render_thread_observer.h"

namespace opera {

class OperaRenderProcessObserver : public content::RenderThreadObserver {
 public:
  OperaRenderProcessObserver();
  virtual ~OperaRenderProcessObserver();

 private:
  DISALLOW_COPY_AND_ASSIGN(OperaRenderProcessObserver);
};

}  // namespace opera

#endif  // CHILL_RENDERER_OPERA_RENDER_PROCESS_OBSERVER_H_
