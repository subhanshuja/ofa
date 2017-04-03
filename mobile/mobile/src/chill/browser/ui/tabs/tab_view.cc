// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/ui/tabs/tab_view.h"

#include <algorithm>

#include "chill/browser/ui/tabs/input.h"
#include "chill/browser/ui/tabs/tab_image_quad.h"
#include "chill/browser/ui/tabs/tab_selector_renderer.h"
#include "chill/browser/ui/tabs/upload_queue.h"
#include "common/opengl_utils/gl_mipmap.h"
#include "common/opengl_utils/gl_texture.h"

namespace opera {
namespace tabui {

TabView::TabView(int id,
                 bool is_dashboard,
                 bool is_private,
                 TabSelectorRenderer* host)
    : id_(id),
      is_dashboard_(is_dashboard),
      is_private_(is_private),
      host_(host),
      screenshot_(std::make_shared<TabImageQuad>()),
      chrome_(std::make_shared<TabImageQuad>()) {}

TabView::~TabView() {}

// Access to the underlying screenshot image.
const std::shared_ptr<GLMipMap> TabView::mipmap() const {
  return screenshot_->mipmap();
}

void TabView::Create(const std::shared_ptr<GLMipMap>& screen_shot,
                     const std::shared_ptr<GLMipMap>& chrome) {
  screenshot_->Create(screen_shot);
  chrome_->Create(chrome);
}

void TabView::UpdateScreenshot(const std::shared_ptr<GLMipMap>& screen_shot) {
  screenshot_->Update(screen_shot);
}

void TabView::UpdateChrome(const std::shared_ptr<GLMipMap>& chrome) {
  chrome_->Update(chrome);
}

void TabView::Draw(gfx::Vector2dF offset,
                   gfx::Vector2dF scale,
                   gfx::Vector4dF color,
                   gfx::Vector2dF cropping,
                   float chrome_opacity,
                   float dimmed) {
  // Scroll the position of the page if we're showing the chrome.
  if (chrome_opacity > 0) {
    offset.set_y(offset.y() - GetChromeScale().y());

    // The omnibar should be attached to the top of the page.
    chrome_->Draw(GetChromeOffset(offset, scale), GetChromeScale(scale),
                  gfx::Vector4dF(1, 1, 1, chrome_opacity), gfx::Vector2dF(1, 1),
                  dimmed, host_);
  }

  screenshot_->Draw(offset, scale, color, cropping, dimmed, host_);
}

bool TabView::IsVisible(gfx::Vector2dF offset, gfx::Vector2dF scale) {
  offset.set_y(offset.y() - GetChromeScale().y());
  return screenshot_->IsVisible(offset, scale, host_->projection_scale());
}

bool TabView::CheckHit(gfx::Vector2dF offset, gfx::Vector2dF input) {
  gfx::Vector2dF scale, chrome_offset, chrome_scale;
  GetHitAreas(&offset, &scale, &chrome_offset, &chrome_scale);

  // The bounding box is the union of the tab and chrome boxes.
  return input::Inside(input, offset - scale, chrome_offset + chrome_scale);
}

bool TabView::CheckHitClose(gfx::Vector2dF offset, gfx::Vector2dF input) {
  gfx::Vector2dF scale, chrome_offset, chrome_scale;
  GetHitAreas(&offset, &scale, &chrome_offset, &chrome_scale);

  // Only the chrome area is important here, but limit the width of the close
  // area to be a rectangle on the right side, i.e. use the same width as the
  // height.
  gfx::Vector2dF min = chrome_offset - chrome_scale;
  gfx::Vector2dF max = chrome_offset + chrome_scale;

  min.set_x(max.x() - (max.y() - min.y()));

  // Make the hit area a little bit bigger than the visible area. This will make
  // it easier to tap on devices with small screens.
  gfx::Vector2dF margin(chrome_scale.y(), chrome_scale.y());
  min -= margin;
  max += margin;

  return input::Inside(input, min, max);
}

void TabView::RequestUpload(VisibleSize visible_size,
                            bool portrait_mode,
                            const std::shared_ptr<TabImageQuad>& image_quad) {
  // Adjust target levels for image_quad. Original resolution of an image quad
  // is approximately same as screen resolution. Shaving off a level will reduce
  // uploaded resolution by a factor of two and size on GPU by a factor of four.
  const size_t min_levels = 2;
  size_t levels = image_quad->mipmap()->original_levels();
  size_t offset = 0;
  switch (visible_size) {
    // If not visible, upload just a few levels to avoid showing black frames if
    // visibility changes.
    case kZeroSize:
      offset = levels;
      break;

    case kSmallSize:
      offset = portrait_mode ? 3 : 4;
      break;

    case kBigSize:
      offset = portrait_mode ? 1 : 2;
      break;

    case kZoomedSize:
      offset = portrait_mode ? 0 : 1;
      break;
  }
  if (host_->is_low_res_device() && offset > 0)
    --offset;
  size_t target_levels =
      levels > offset + min_levels ? levels - offset : min_levels;

  DCHECK_LE(target_levels, levels);
  DCHECK_LE(std::min(levels, static_cast<size_t>(2)), target_levels);

  image_quad->SetTargetLevels(target_levels);
  host_->GetUploadQueue()->Enqueue(image_quad);
}

void TabView::RequestUpload(VisibleSize visible_size, bool portrait_mode) {
  RequestUpload(visible_size, portrait_mode, screenshot_);
  RequestUpload(visible_size, portrait_mode, chrome_);
}

void TabView::RequestUpload() {
  screenshot_->SetTargetLevels(screenshot_->mipmap()->levels());
  screenshot_->Upload(false);
}

gfx::Vector2dF TabView::Cropping() const {
  // Always aim to show screenshots in portrait mode.

  // If we have screenshots that were taken in landscape mode then we crop the
  // image to only show the left part of it.
  size_t mipmap_w = mipmap()->original_width();
  size_t mipmap_h = mipmap()->original_height();
  float x = 1;
  if (mipmap_w > mipmap_h) {
    if (host_->portrait_mode()) {
      size_t screen_w = host_->screen_width();
      size_t screen_h = host_->screen_height();
      size_t ornament =
          host_->viewport_top_height() + host_->viewport_bottom_height();

      float mipmap_ratio = mipmap_h / static_cast<float>(mipmap_w);
      float viewport_ratio = screen_w / static_cast<float>(screen_h - ornament);

      x = mipmap_ratio * viewport_ratio;
    } else {
      x = host_->is_rtl() ? -host_->projection_scale().x()
                          : host_->projection_scale().x();
    }
  }

  return gfx::Vector2dF(x, 1);
}

gfx::Vector2dF TabView::GetChromeScale() const {
  return host_->GetTabChromeScale();
}

gfx::Vector2dF TabView::GetChromeOffset(gfx::Vector2dF offset,
                                              gfx::Vector2dF scale) const {
  offset.set_y(offset.y() + (1 + GetChromeScale().y()) * (scale.y() / 2));
  return offset;
}

gfx::Vector2dF TabView::GetChromeScale(gfx::Vector2dF scale) const {
  gfx::Vector2dF result = GetChromeScale();
  result.set_x(result.x() * scale.x());
  result.set_y(result.y() * scale.y());
  return result;
}

void TabView::GetHitAreas(gfx::Vector2dF* page_offset,
                          gfx::Vector2dF* page_scale,
                          gfx::Vector2dF* chrome_offset,
                          gfx::Vector2dF* chrome_scale) {
  // Scroll the position of the page if we're showing the chrome.
  page_offset->set_y(page_offset->y() - GetChromeScale().y());

  // Get the scale of the page in idle mode.
  *page_scale = gfx::Vector2dF(1.0, host_->viewport_height_scale());

  // The omnibar should be attached to the top of the page.
  *chrome_offset = GetChromeOffset(*page_offset, *page_scale);

  // The chrome already has a scale to make to match it's original pixel
  // resolution. It still needs to be scaled together with the page however.
  *chrome_scale = GetChromeScale(*page_scale);

  // Convert to normalized input scale.
  chrome_scale->Scale(0.5f);
  page_scale->Scale(0.5f);
}

}  // namespace tabui
}  // namespace opera
