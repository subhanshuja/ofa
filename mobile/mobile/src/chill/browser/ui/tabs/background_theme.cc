// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/ui/tabs/background_theme.h"

#include "base/logging.h"
#include "base/rand_util.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/geometry/vector3d_f.h"

#include "chill/browser/ui/tabs/shaders.h"
#include "chill/browser/ui/tabs/tab_selector_renderer.h"
#include "common/opengl_utils/gl_geometry.h"
#include "common/opengl_utils/gl_mipmap.h"
#include "common/opengl_utils/gl_shader.h"
#include "common/opengl_utils/gl_texture.h"

namespace opera {
namespace tabui {

namespace {
// Size of the diffusion pattern. Only just large enough to not be obviously
// repeating when tiled.
const int kPatternSize = 64;

// Normal mode background color.
const gfx::Vector3dF kNormalBackgroundColor(0x4d / 255.0f,
                                            0x50 / 255.0f,
                                            0x5f / 255.0f);

// Private mode background color.
const gfx::Vector3dF kPrivateBackgroundColor(0x20 / 255.0f,
                                             0x21 / 255.0f,
                                             0x2a / 255.0f);
}  // namespace

BackgroundTheme::BackgroundTheme() : background_interpolation_(0.0f) {}

bool BackgroundTheme::OnCreate() {
  do {
    // Set up the rendering resources.
    const char* kVertexDescription = "p2f";
    shader_ = GLShader::Create(kColorVertexShader, kColorFragmentShader,
                               kVertexDescription);
    if (!shader_) {
      LOG(ERROR) << "Failed to load the background theme shader.";
      break;
    }

    SetBackgroundColor(kNormalBackgroundColor);
    return true;
  } while (false);

  // It's all or nothing.
  shader_.reset();
  return false;
}

void BackgroundTheme::OnGLContextLost() {
  if (shader_)
    shader_->OnGLContextLost();
}

void BackgroundTheme::Draw(float background_interpolation,
                           TabSelectorRenderer* renderer) {
  if (!shader_)
    return;

  if (background_interpolation != background_interpolation_) {
    background_interpolation_ = background_interpolation;

    gfx::Vector3dF background_color;
    if (background_interpolation_ <= 0) {
      background_color = kNormalBackgroundColor;
    } else if (background_interpolation_ >= 1) {
      background_color = kPrivateBackgroundColor;
    } else {
      background_color = (kPrivateBackgroundColor - kNormalBackgroundColor);
      background_color.Scale(background_interpolation);
      background_color += kNormalBackgroundColor;
    }
    SetBackgroundColor(background_color);
  }

  shader_->Activate();
  gfx::Vector2dF proj(1.f, 1.f);
  // NOTE: solid_quad() is [-0.5; 0.5], we want [-1; 1]
  gfx::Vector2dF vert(2.f, 2.f);
  gfx::Vector2dF offs(0.f, 0.f);
  shader_->SetUniform("u_projection_scale", proj);
  shader_->SetUniform("u_vertex_scale", vert);
  shader_->SetUniform("u_vertex_offset", offs);
  shader_->SetUniform("u_color", background_color_);
  renderer->solid_quad()->Draw();
}

void BackgroundTheme::SetBackgroundColor(gfx::Vector3dF col) {
  background_color_.set_x(col.x());
  background_color_.set_y(col.y());
  background_color_.set_z(col.z());
  background_color_.set_w(1.f);
}

}  // namespace tabui
}  // namespace opera
