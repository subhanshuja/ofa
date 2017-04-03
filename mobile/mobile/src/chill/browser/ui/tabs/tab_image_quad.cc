// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/ui/tabs/tab_image_quad.h"

#include "chill/browser/ui/tabs/tab_selector_renderer.h"
#include "common/opengl_utils/gl_geometry.h"
#include "common/opengl_utils/gl_shader.h"
#include "common/opengl_utils/gl_texture.h"

namespace opera {
namespace tabui {

TabImageQuad::TabImageQuad() : ImageQuad(false) {}

// Check if any part of the rectangle overlap the viewport (-1,-1)..(1,1).
bool TabImageQuad::IsVisible(const gfx::Vector2dF& offset,
                             const gfx::Vector2dF& scale,
                             const gfx::Vector2dF& projection_scale) const {
  gfx::Vector2dF topLeft = offset - scale;
  gfx::Vector2dF bottomRight = offset + scale;
  float viewport_width = 1.0f / std::abs(projection_scale.x());
  float viewport_height = 1.0f / std::abs(projection_scale.y());

  return (topLeft.x() < viewport_width) && (topLeft.y() < viewport_height) &&
         (bottomRight.x() > -viewport_width) &&
         (bottomRight.y() > -viewport_height);
}

void TabImageQuad::Draw(const gfx::Vector2dF& offset,
                        const gfx::Vector2dF& scale,
                        const gfx::Vector4dF& color,
                        const gfx::Vector2dF& cropping,
                        float dimmer,
                        TabSelectorRenderer* renderer) {
  texture()->Activate();

  GLShader* shader = renderer->image_shader();

  gfx::Vector2dF projection_scale = renderer->projection_scale();

  // Revert any mirroring for quad.
  gfx::Vector2dF vertex_scale = scale;
  vertex_scale.Scale(std::copysign(1.f, projection_scale.x()),
                     std::copysign(1.f, projection_scale.y()));

  if (shader) {
    shader->Activate();
    shader->SetUniform("u_projection_scale", projection_scale);
    shader->SetUniform("u_vertex_offset", offset);
    shader->SetUniform("u_vertex_scale", vertex_scale);
    shader->SetUniform("u_color", color);
    shader->SetUniform("u_cropping", cropping);
    shader->SetUniform("u_dimmer", dimmer);
    shader->SetUniform("u_image", 0);

    GLGeometry* geometry = renderer->image_quad();
    if (geometry)
      geometry->Draw();
  }
}

}  // namespace tabui
}  // namespace opera
