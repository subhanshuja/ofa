// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/ui/tabs/private_mode_placeholder.h"

#include "base/logging.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"

#include "chill/browser/ui/tabs/selector_view.h"
#include "chill/browser/ui/tabs/tab_selector_renderer.h"
#include "common/opengl_utils/gl_mipmap.h"
#include "common/opengl_utils/gl_texture.h"

namespace opera {
namespace tabui {

PrivateModePlaceholder::PrivateModePlaceholder() : ImageQuad(true) {
  // Create a default mipmap to always have something to draw.
  SkBitmap bitmap;
  if (!bitmap.tryAllocPixels(SkImageInfo::MakeN32Premul(16, 16))) {
    // Can not happen on android since malloc (almost) never fails.
    NOTREACHED();
  }
  bitmap.eraseARGB(0, 0, 0, 255);
  Create(GLMipMap::Create(bitmap, false, GLMipMap::kCompressAlways));
}

void PrivateModePlaceholder::OnCreate(jobject bitmap,
                                      TabSelectorRenderer* renderer) {
  Create(GLMipMap::Create(bitmap, true, GLMipMap::kCompressAlways));

  // Get it to normalized coordinates.
  size_t mipmap_w = mipmap()->original_width();
  size_t mipmap_h = mipmap()->original_height();
  mipmap_ratio_ = mipmap_h / static_cast<float>(mipmap_w);
  AdjustScale(renderer->screen_width(), renderer->screen_height());
}

void PrivateModePlaceholder::OnSurfaceChanged(int width, int height) {
  AdjustScale(width, height);
}

bool PrivateModePlaceholder::IsVisible(float position) const {
  return position < scale_.x();
}

void PrivateModePlaceholder::Draw(float position,
                                  float dimmer,
                                  TabSelectorRenderer* renderer) {
  if (!texture() || !IsVisible(position))
    return;

  texture()->Activate();

  // Revert any mirroring for quad.
  gfx::Vector2dF vertex_scale = scale_;
  vertex_scale.Scale(std::copysign(1.f, renderer->projection_scale().x()),
                     std::copysign(1.f, renderer->projection_scale().y()));

  GLShader* shader = renderer->image_shader();
  if (!shader)
    return;

  shader->Activate();
  shader->SetUniform("u_projection_scale", renderer->projection_scale());
  shader->SetUniform("u_vertex_offset", gfx::Vector2dF(position, 0.1f));
  shader->SetUniform("u_vertex_scale", vertex_scale);
  shader->SetUniform("u_cropping", gfx::Vector2dF(1.0f, 1.0f));
  shader->SetUniform("u_color", gfx::Vector4dF(1.0f, 1.0f, 1.0f, dimmer));
  shader->SetUniform("u_image", 0);

  GLGeometry* geometry = renderer->image_quad();
  if (!geometry)
    return;

  geometry->Draw();
}

void PrivateModePlaceholder::AdjustScale(int screen_w, int screen_h) {
  bool portrait_mode = screen_w < screen_h;
  float viewport_ratio =
    portrait_mode ? (screen_w / static_cast<float>(screen_h))
                  : (screen_h / static_cast<float>(screen_w));

  scale_ = gfx::Vector2dF(1.0f, mipmap_ratio_ * viewport_ratio);

  // Scale it up to taste.
  scale_.Scale(portrait_mode ? 1.5f : 1.8f);
}

}  // namespace tabui
}  // namespace opera
