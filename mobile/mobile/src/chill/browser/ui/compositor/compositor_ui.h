// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014-2016 Opera Software AS. All rights reserved.
//
// This file is an original work developed by Opera Software AS

#ifndef CHILL_BROWSER_UI_COMPOSITOR_COMPOSITOR_UI_H_
#define CHILL_BROWSER_UI_COMPOSITOR_COMPOSITOR_UI_H_

#include <jni.h>

#include "base/memory/ref_counted.h"
#include "cc/layers/ui_resource_layer.h"
#include "cc/layers/layer.h"
#include "ui/gfx/geometry/size.h"

namespace opera {

// Creates UI layers in the chromium browser compositor that can be controlled
// from the Java UI.
class CompositorUi {
 public:
  CompositorUi();

  // Updates the bitmap for the top controls layer.
  void SetTopControlsLayerBitmap(jobject bitmap);

  // Updates the dashboard bitmap that is used as a static background image in
  // transitions.
  void SetDashboardLayerBitmap(jobject bitmap);

  // Set the active content layer.
  void SetContentLayer(jobject web_contents);

  // Updates the offset for the top controls and content layers.
  void OffsetTopControls(float y_offset);
  void OffsetContent(float y_offset);

  // Potentially show a static background bitmap and prepares for animating the
  // content layer. Decide if it's the content or static image layer that should
  // be animated. Disables TopControl animation.
  void BeginTransition(bool show_static_image, bool animate_content);

  // Update the content layer animation.
  void UpdateTransition(float fraction);

  // Hide the static background bitmap, restores the content layer position and
  // enables TopControl animations again.
  void EndTransition();

 private:
  scoped_refptr<cc::Layer> content_layer_;
  scoped_refptr<cc::UIResourceLayer> top_bar_layer_;
  scoped_refptr<cc::UIResourceLayer> dashboard_layer_;

  float content_offset_;
  float top_controls_offset_;
  bool transition_active_;
  bool animate_content_;

  // Create a new layer and initialize properties.
  scoped_refptr<cc::UIResourceLayer> CreateUILayer();

  // Convert from java to skia bitmap and update the bounding rectangle for
  // the layer. Will return true if updating the bitmap succeeded.
  bool UpdateUILayerBitmap(jobject bitmap,
                           scoped_refptr<cc::UIResourceLayer> layer);

  void RestoreContentLayerPosition();
};

}  // namespace opera

#endif  // CHILL_BROWSER_UI_COMPOSITOR_COMPOSITOR_UI_H_
