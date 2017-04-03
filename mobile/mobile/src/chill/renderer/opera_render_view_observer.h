// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_RENDERER_OPERA_RENDER_VIEW_OBSERVER_H_
#define CHILL_RENDERER_OPERA_RENDER_VIEW_OBSERVER_H_

#include "content/public/renderer/render_view_observer.h"

namespace content {
class RenderView;
}  // namespace content

namespace opera {

class OperaRenderViewObserver : public content::RenderViewObserver {
 public:
  explicit OperaRenderViewObserver(content::RenderView* render_view);
  virtual ~OperaRenderViewObserver();

 private:
  void OnDestruct() override;
  bool OnMessageReceived(const IPC::Message& message) override;

  void OnGetWebApplicationInfo();
};

}  // namespace opera

#endif  // CHILL_RENDERER_OPERA_RENDER_VIEW_OBSERVER_H_
