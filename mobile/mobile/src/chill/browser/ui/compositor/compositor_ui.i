// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014-2016 Opera Software AS. All rights reserved.
//
// This file is an original work developed by Opera Software AS

%{
#include "chill/browser/ui/compositor/compositor_ui.h"
%}

namespace opera {

class CompositorUi {
 public:
  CompositorUi();

  void SetTopControlsLayerBitmap(jobject bitmap);
  void SetDashboardLayerBitmap(jobject bitmap);
  void SetContentLayer(jobject web_contents);
  void OffsetTopControls(float y_offset);
  void OffsetContent(float y_offset);
  void BeginTransition(bool show_static_image, bool animate_content);
  void UpdateTransition(float fraction);
  void EndTransition();
};

}  // namespace opera
