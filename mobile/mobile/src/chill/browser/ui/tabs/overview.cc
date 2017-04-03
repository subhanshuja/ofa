// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/ui/tabs/overview.h"

#include "base/logging.h"
#include "chill/browser/ui/tabs/input.h"
#include "chill/browser/ui/tabs/shaders.h"
#include "chill/browser/ui/tabs/selector_view.h"
#include "chill/browser/ui/tabs/tab_selector_renderer.h"
#include "chill/browser/ui/tabs/tab_view.h"

namespace opera {
namespace tabui {

VisibleTabsContainer Overview::Cull(float scroll_position,
                                    float camera_target,
                                    float dimmer,
                                    TabSelectorRenderer* host) {
  VisibleTabsContainer result;

  // The tabs are too small and shouldn't be shown in landscape mode.
  if (!host->portrait_mode())
    return result;

  // Let the overall size of a miniature tab be 10% of the viewport.
  const float kViewScale = 0.1f;
  const float kSpaceBetweenNormalAndPrivateMode =
      SelectorView::GetSpaceBetweenNormalAndPrivateMode() * kViewScale;

  float height_scale = host->viewport_height_scale();

  // Keep a position offset that is updated for each tab that is drawn.
  gfx::Vector2dF offset(scroll_position * kViewScale, -0.65f);

  // Calculate the bounding boxes as we go.
  // TODO(peterp): Would be better if we could calculate the bounding boxes in
  // the hit test function instead.
  bounding_box_normal_min_ = gfx::Vector2dF(FLT_MAX, FLT_MAX);
  bounding_box_normal_max_ = gfx::Vector2dF(-FLT_MAX, -FLT_MAX);
  bounding_box_private_min_ = gfx::Vector2dF(FLT_MAX, FLT_MAX);
  bounding_box_private_max_ = gfx::Vector2dF(-FLT_MAX, -FLT_MAX);

  // Draw as many boxes as there are tabs.
  size_t num_tabs = host->tabs().size();

  // In private mode and unless we already have some private tabs then add an
  // extra fake tab at the end.
  bool add_fake_tab = false;
  if (host->private_mode() && !host->private_tab_count()) {
    ++num_tabs;
    add_fake_tab = true;
  }

  for (size_t i = 0; i < num_tabs; ++i) {
    // tabs:             [0]  [1]  [2]  [3]  [4]  [5]  [6]  [7]  [8]  [9]
    // fixed_pos:        0.0  1.0  2.0  3.0  4.0  5.0  6.0  7.0  8.0  9.0
    //
    // camera_target:                      3.7
    //
    // kInfluenceRadius:            |.......|.......|
    // influence:        0.0  0.0  0.0  0.53 0.8  0.13 0.0  0.0  0.0  0.0
    //
    // scale_factor:     1.0  1.0  1.0  1.34 1.4  1.07 1.0  1.0  1.0  1.0

    // Calculate the zoom factor on the focused and adjacent tabs.
    const float kNormalScale = 1.0f;
    const float kZoomedScale = 1.5f;
    const float kInfluenceRadius = 1.5f;
    float camera_delta = camera_target - i;
    float influence = cc::MathUtil::ClampToRange(
        static_cast<float>(fabs(camera_delta)), 0.0f, kInfluenceRadius);
    influence = 1.0f - influence / kInfluenceRadius;
    float scale_factor =
        kNormalScale + (kZoomedScale - kNormalScale) * influence;

    // Scale the radius with the zoom factor.
    float scale = kViewScale * 0.5f * scale_factor;

    // The quads are drawn from the center position and the distance between
    // two miniature tabs are radius from this and the next plus an extra space
    // in the middle.
    //
    // +-----+
    // |     | +---+
    // |  x  | | x |
    // |     | +---+
    // +-----+
    //                a = 'scale' for tab[i]
    //    |--|-|-|    b = 'kSpaceBetweenMiniTabs'
    //     a  b c     c = 'scale' for tab[i+1]
    //
    if (i != 0) {
      // This is the 'b'+'c' part being added to the offset.
      const float kSpaceBetweenMiniTabs =
          0.005f;  // TODO(peterp): regular gap size * overview scale
      offset.set_x(offset.x() + scale + kSpaceBetweenMiniTabs);
    }

    // There's a gap between the row of normal and private mode tabs.
    if (i == host->public_tab_count())
      offset.set_x(offset.x() + kSpaceBetweenNormalAndPrivateMode);

    float draw_scale = scale * 2.0f;
    gfx::Vector2dF draw_scale_2d(draw_scale, draw_scale * height_scale);

    // Special mode to draw the extra fake "private mode" tab.
    if (add_fake_tab && (i >= (num_tabs - 1))) {
      // TODO(peterp): hack
      offset.set_x(offset.x() + kSpaceBetweenNormalAndPrivateMode);

      GLShader* shader = host->color_shader();
      if (shader) {
        shader->Activate();
        shader->SetUniform("u_projection_scale", host->projection_scale());
        shader->SetUniform("u_vertex_offset", offset);
        shader->SetUniform("u_vertex_scale", draw_scale_2d);
        shader->SetUniform("u_color", gfx::Vector4dF(1.0f, 1.0f, 1.0f, 1.0f));

        GLGeometry* geometry = host->outline_quad();
        if (geometry)
          geometry->Draw();
      }
    } else {
      // If we're in normal mode then draw private mode tabs in a dark shade
      // without showing the actual content.
      float d = 0.0f;
      if (!host->private_mode() && (i >= host->public_tab_count())) {
        d = dimmer;
      }

      const auto& tab = host->tabs()[i];

      // Let the influence affect the intensity as well.
      const float kMinIntensity = 0.5f;
      const float kMaxIntensity = 1.0f;
      float intensity =
          kMinIntensity + (kMaxIntensity - kMinIntensity) * influence;
      gfx::Vector4dF color(intensity, intensity, intensity, 1.0f);

      float chrome_opacity = 0.0f;
      if (tab->IsVisible(offset, draw_scale_2d))
        result.push_back({tab, offset, draw_scale_2d, color, tab->Cropping(),
                          chrome_opacity, d});

      // Update the bounding box.
      gfx::Vector2dF tab_scale = TabView::GetTabScale();
      tab_scale.Scale(draw_scale);
      if (host->private_tab_count() && i >= host->public_tab_count()) {
        bounding_box_private_min_.SetToMin(offset - tab_scale);
        bounding_box_private_max_.SetToMax(offset + tab_scale);
      } else {
        bounding_box_normal_min_.SetToMin(offset - tab_scale);
        bounding_box_normal_max_.SetToMax(offset + tab_scale);
      }

      // Adjust the position to make room for the zoomed in tabs.
      // This is the 'a' part being added to the offset from the graph above.
      offset.set_x(offset.x() + scale);
    }
  }

  return result;
}

int Overview::CheckHit(gfx::Vector2dF input, TabSelectorRenderer* host) {
  // Don't even check if we're not visible.
  if (!host->portrait_mode())
    return -1;

  // Check the bounding boxes.

  // Compare the input coordinates against the bounding box.
  gfx::Vector2dF input_view_space =
      input::ConvertToLocal(input, host->projection_scale());

  float x = input_view_space.x() - bounding_box_normal_min_.x();
  float y = input_view_space.y() - bounding_box_normal_min_.y();
  float w = bounding_box_normal_max_.x() - bounding_box_normal_min_.x();
  float h = bounding_box_normal_max_.y() - bounding_box_normal_min_.y();

  int num_normal_tabs = host->public_tab_count();

  if ((x > 0) && (x < w) && (y > 0) && (y < h)) {
    // Don't want to accidentally hit the space between tabs so instead use the
    // whole width divided by number of tabs.
    return static_cast<int>((x / w) * num_normal_tabs);
  }

  if (host->private_tab_count()) {
    x = input_view_space.x() - bounding_box_private_min_.x();
    y = input_view_space.y() - bounding_box_private_min_.y();
    w = bounding_box_private_max_.x() - bounding_box_private_min_.x();
    h = bounding_box_private_max_.y() - bounding_box_private_min_.y();

    if ((x > 0) && (x < w) && (y > 0) && (y < h)) {
      int num_private_tabs = host->private_tab_count();

      // Don't want to accidentally hit the space between tabs so instead use
      // the whole width divided by number of tabs.
      return static_cast<int>((x / w) * num_private_tabs) +
             host->public_tab_count();
    }
  }

  return -1;
}

}  // namespace tabui
}  // namespace opera
