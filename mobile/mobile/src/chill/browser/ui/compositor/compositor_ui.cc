// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014-2016 Opera Software AS. All rights reserved.
//
// This file is an original work developed by Opera Software AS

#include "chill/browser/ui/compositor/compositor_ui.h"

#include "cc/trees/layer_tree_host.h"
#include "content/public/browser/android/content_view_core.h"
#include "content/public/browser/web_contents.h"

#include "chill/browser/skia/skia_utils.h"

namespace opera {

CompositorUi::CompositorUi()
    : content_offset_(0),
      top_controls_offset_(0),
      transition_active_(false),
      animate_content_(true) {
  top_bar_layer_ = CreateUILayer();
  dashboard_layer_ = CreateUILayer();
}

void CompositorUi::SetTopControlsLayerBitmap(jobject bitmap) {
  if (top_bar_layer_) {
    // Only turn on the layer if the bitmap update succeeded.
    top_bar_layer_->SetIsDrawable(UpdateUILayerBitmap(bitmap, top_bar_layer_));
  }
}

void CompositorUi::SetDashboardLayerBitmap(jobject bitmap) {
  if (dashboard_layer_) {
    // Leave the layer turned off until the animation is triggered.
    UpdateUILayerBitmap(bitmap, dashboard_layer_);
  }
}

void CompositorUi::SetContentLayer(jobject web_contents) {
  if (transition_active_ && content_layer_) {
    RestoreContentLayerPosition();
  }

  content_layer_ = content::WebContents::FromJavaWebContents(
                       base::android::ScopedJavaLocalRef<jobject>(
                           base::android::AttachCurrentThread(), web_contents))
                       ->GetNativeView()
                       ->GetLayer();
  if (content_layer_) {
    // Detach the top bar layer from the previous content view core and attach
    // to the new one instead.
    // The new ui should be independently transformed on top of the content
    // layer so we add them as a separate child layers to the same root layer.
    // NOTE: This assumes that the content layer is the first layer attached to
    // the root layer.
    scoped_refptr<cc::Layer> root_layer = content_layer_->RootLayer();

    // Top bar should always be on top the content layer.
    root_layer->InsertChild(top_bar_layer_, 1);

    // Default to position the static behind the content layer. This will be
    // reordered when a transition is started.
    root_layer->InsertChild(dashboard_layer_, 0);
  }
}

void CompositorUi::OffsetContent(float y_offset) {
  content_offset_ = y_offset;

  if (content_layer_ && !transition_active_) {
    // Translate the content layer.
    gfx::Transform transform;
    transform.Translate(SkFloatToMScalar(0), SkFloatToMScalar(y_offset));
    content_layer_->SetTransform(transform);
  }
}

void CompositorUi::OffsetTopControls(float y_offset) {
  top_controls_offset_ = y_offset;

  if (top_bar_layer_ && !transition_active_) {
    // Translate the top bar layer partially outside the screen.
    gfx::Transform transform;
    transform.Translate(SkDoubleToMScalar(0.0), SkDoubleToMScalar(y_offset));
    top_bar_layer_->SetTransform(transform);
  }
}

void CompositorUi::BeginTransition(bool show_static_image,
                                   bool animate_content) {
  // Turn off top bar animations and make the static background bitmap visible.
  transition_active_ = true;
  animate_content_ = animate_content;

  if (dashboard_layer_) {
    dashboard_layer_->SetIsDrawable(show_static_image);

    // Reorder the static screen layer to either be below or in front of the
    // content layer.
    if (content_layer_) {
      scoped_refptr<cc::Layer> root_layer = content_layer_->RootLayer();
      if (top_bar_layer_)
        root_layer->InsertChild(top_bar_layer_, 1);
      root_layer->InsertChild(dashboard_layer_, animate_content ? 0 : 2);
    }
  }
}

void CompositorUi::UpdateTransition(float fraction) {
  // There's no point in doing transitions if there's no content layer or tree.
  if (!transition_active_ || !content_layer_)
    return;
  cc::LayerTree* layer_tree = content_layer_->GetLayerTree();
  if (!layer_tree)
    return;

  // Figure out the total height of the viewport.
  // Then convert the fraction to an actual y offset for the layers.
  float y_offset = fraction * layer_tree->device_viewport_size().height();
  float content_offset = animate_content_ ? y_offset : 0.0f;
  float image_offset = animate_content_ ? 0.0f : y_offset;

  if (dashboard_layer_) {
    // The static screen is in a static position.
    gfx::Transform transform;
    transform.Translate(SkFloatToMScalar(0), SkFloatToMScalar(image_offset));
    dashboard_layer_->SetTransform(transform);
    dashboard_layer_->SetNeedsDisplay();
  }

  if (top_bar_layer_) {
    // The top bar is attached on top of the content layer.
    gfx::Transform transform;
    transform.Translate(SkFloatToMScalar(0), SkFloatToMScalar(content_offset));
    top_bar_layer_->SetTransform(transform);
    top_bar_layer_->SetNeedsDisplay();
  }

  if (content_layer_) {
    // Move the content layer to the new position and offset it a bit extra
    // to allow for the top bar.
    gfx::Transform transform;
    transform.Translate(SkFloatToMScalar(0),
                        SkFloatToMScalar(content_offset + content_offset_));
    content_layer_->SetTransform(transform);
    content_layer_->SetNeedsDisplay();
  }

  // Trigger a commit and refresh for the impl/compositor side of layer tree
  // host.
  layer_tree->SetNeedsCommit();
}

void CompositorUi::EndTransition() {
  // Allow for top bar animations again and hide the static background bitmap.
  transition_active_ = false;

  if (dashboard_layer_) {
    // Always hide the static image.
    dashboard_layer_->SetIsDrawable(false);
  }

  if (content_layer_) {
    RestoreContentLayerPosition();
  }
}

scoped_refptr<cc::UIResourceLayer> CompositorUi::CreateUILayer() {
  scoped_refptr<cc::UIResourceLayer> layer = cc::UIResourceLayer::Create();
  layer->SetIsDrawable(false);
  return layer;
}

bool CompositorUi::UpdateUILayerBitmap(
    jobject bitmap,
    scoped_refptr<cc::UIResourceLayer> layer) {
  if (bitmap != NULL) {
    std::unique_ptr<SkBitmap> sk_bitmap(
        SkiaUtils::CreateFromJavaBitmap(bitmap));
    if (sk_bitmap) {
      sk_bitmap->setImmutable();
      layer->SetBitmap(*sk_bitmap);
      layer->SetBounds(gfx::Size(sk_bitmap->width(), sk_bitmap->height()));
      return true;
    }
  }
  return false;
}

void CompositorUi::RestoreContentLayerPosition() {
  gfx::Transform transform;
  transform.Translate(SkFloatToMScalar(0), SkFloatToMScalar(content_offset_));
  content_layer_->SetTransform(transform);
  content_layer_->SetNeedsDisplay();
}

}  // namespace opera
