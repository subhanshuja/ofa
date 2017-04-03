// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_RENDERER_OPERA_RENDER_FRAME_OBSERVER_H_
#define CHILL_RENDERER_OPERA_RENDER_FRAME_OBSERVER_H_

#include <string>

#include "content/public/renderer/render_frame_observer.h"

class OperaRenderFrameObserver : public content::RenderFrameObserver {
 public:
  explicit OperaRenderFrameObserver(content::RenderFrame* render_frame);
  ~OperaRenderFrameObserver() override;

 private:
  // RenderFrameObserver implementation.
  void OnDestruct() override;
  bool OnMessageReceived(const IPC::Message& message) override;

  // IPC handlers
  void OnAppBannerPromptRequest(int request_id, const std::string& platform);

  DISALLOW_COPY_AND_ASSIGN(OperaRenderFrameObserver);
};

#endif  // CHILL_RENDERER_OPERA_RENDER_FRAME_OBSERVER_H_
