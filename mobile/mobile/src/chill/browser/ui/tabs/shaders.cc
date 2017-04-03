// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/ui/tabs/shaders.h"

namespace opera {
namespace tabui {

// Simplify the string handling for the shader source code.
#define GLSL_SRC(text) #text

const char* kImageVertexShader = GLSL_SRC(
  attribute vec2 a_position;
  attribute vec2 a_tex_coord0;

  uniform vec2 u_projection_scale;
  uniform vec2 u_vertex_scale;
  uniform vec2 u_vertex_offset;
  uniform vec2 u_cropping;

  varying vec2 v_tex_coord0;

  void main() {
    // Simple 2d transform.
    vec2 position = (a_position * u_vertex_scale) + u_vertex_offset;
    position *= u_projection_scale;

    v_tex_coord0 = a_tex_coord0 * u_cropping;

    gl_Position = vec4(position, 0.0, 1.0);
  }
);

const char* kImageFragmentShader = GLSL_SRC(
  precision mediump float;

  uniform vec4 u_color;
  uniform sampler2D u_image;
  uniform float u_dimmer;

  varying vec2 v_tex_coord0;

  void main() {
    vec4 color = texture2D(u_image, v_tex_coord0) * u_color;
    gl_FragColor = mix(color, vec4(0.0, 0.0, 0.0, 0.2), u_dimmer);
  }
);

const char* kImageFragmentShaderLOD =
  "#version 100\n"
  "#extension GL_EXT_shader_texture_lod : require\n"
GLSL_SRC(
  precision mediump float;

  uniform vec4 u_color;
  uniform sampler2D u_image;
  uniform float u_dimmer;

  varying vec2 v_tex_coord0;

  void main() {
    vec4 color = texture2DLodEXT(u_image, v_tex_coord0, -1.0) * u_color;
    gl_FragColor = mix(color, vec4(0.0, 0.0, 0.0, 0.2), u_dimmer);
  }
);

const char* kColorVertexShader = GLSL_SRC(
  attribute vec2 a_position;

  uniform vec2 u_projection_scale;
  uniform vec2 u_vertex_scale;
  uniform vec2 u_vertex_offset;

  void main() {
    // Simple 2d transform.
    vec2 position = (a_position * u_vertex_scale) + u_vertex_offset;
    position *= u_projection_scale;

    gl_Position = vec4(position, 0.0, 1.0);
  }
);

const char* kColorFragmentShader = GLSL_SRC(
  precision mediump float;

  uniform vec4 u_color;

  void main() {
    gl_FragColor = u_color;
  }
);

const char* kBackgroundThemeVertexShader = GLSL_SRC(
  attribute vec2 a_position;
  attribute vec3 a_color;

  uniform vec2 u_texture_scale;

  varying vec3 v_color0;
  varying vec2 v_tex_coord0;

  void main() {
    v_color0 = a_color;
    v_tex_coord0 = a_position * u_texture_scale;
    gl_Position = vec4(a_position, 0.0, 1.0);
  }
);

const char* kBackgroundThemeFragmentShader = GLSL_SRC(
  precision mediump float;

  uniform sampler2D u_image;

  varying vec3 v_color0;
  varying vec2 v_tex_coord0;

  void main() {
    // Add some noise to the interpolated vertex colors to hide banding.
    float noise = texture2D(u_image, v_tex_coord0).r;

    const float kSpread = 0.1;
    float diffusion = noise * kSpread;
    diffusion -= kSpread * 0.5;
    diffusion += 1.0;

    gl_FragColor = vec4(v_color0 * diffusion, 1.0);
  }
);

}  // namespace tabui
}  // namespace opera
