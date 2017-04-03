// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_UI_TABS_SHADERS_H_
#define CHILL_BROWSER_UI_TABS_SHADERS_H_

namespace opera {
namespace tabui {

// GLSL shader source code.

// Draw textured geometry.
//
// Attributes:        position, texture_coordinate.
// Vertex uniforms:   projection_scale, vertex_scale, vertex_offset, cropping.
// Fragment uniforms: color, image, dimmer.
extern const char* kImageVertexShader;
extern const char* kImageFragmentShader;
extern const char* kImageFragmentShaderLOD;

// Draw colored geometry.
//
// Attributes:        position.
// Vertex uniforms:   projection_scale, vertex_scale, vertex_offset.
// Fragment uniforms: color.
extern const char* kColorVertexShader;
extern const char* kColorFragmentShader;

// Draw a gradient with modulated (texture) noise.
//
// Attributes:        position, color.
// Vertex uniforms:   texture_scale.
// Fragment uniforms: image.
extern const char* kBackgroundThemeVertexShader;
extern const char* kBackgroundThemeFragmentShader;

}  // namespace tabui
}  // namespace opera

#endif  // CHILL_BROWSER_UI_TABS_SHADERS_H_
