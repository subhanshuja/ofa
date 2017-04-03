// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_UI_TABS_BACKGROUND_THEME_H_
#define CHILL_BROWSER_UI_TABS_BACKGROUND_THEME_H_

#include <memory>

#include "common/opengl_utils/vector4d_f.h"
#include "ui/gfx/geometry/vector3d_f.h"

namespace opera {

class GLGeometry;
class GLMipMap;
class GLShader;
class GLTexture;

namespace tabui {
class TabSelectorRenderer;

// Draw the background of the tab menu UI.
//
// This is currently implemented as drawing a solid color.
class BackgroundTheme {
 public:
  BackgroundTheme();

  // Set up all the render resources needed to draw.
  bool OnCreate();

  // Called if the OS have dropped the OpenGL context.
  // Until the object have been recreated it'll act like a NOP and draw nothing.
  void OnGLContextLost();

  // Draw to the current OpenGL context.
  void Draw(float background_interpolation, TabSelectorRenderer* host);

 private:
  void SetBackgroundColor(gfx::Vector3dF col);

  // GL render resources.
  std::unique_ptr<GLShader> shader_;

  // Interpolation between normal and private mode background colors.
  float background_interpolation_;
  gfx::Vector4dF background_color_;
};

}  // namespace tabui
}  // namespace opera

#endif  // CHILL_BROWSER_UI_TABS_BACKGROUND_THEME_H_
