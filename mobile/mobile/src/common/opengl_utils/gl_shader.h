// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OPENGL_UTILS_GL_SHADER_H_
#define COMMON_OPENGL_UTILS_GL_SHADER_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "common/opengl_utils/opengl.h"
#include "common/opengl_utils/vector4d_f.h"
#include "ui/gfx/geometry/vector2d_f.h"
#include "ui/gfx/geometry/vector3d_f.h"

namespace opera {

// Encapsulates a shader program that can be used when rendering geometry in
// OpenGL.
//
// It holds a vertex and fragment shaders together with a mapping for the
// available uniforms.
class GLShader {
 public:
  ~GLShader();

  // Compile, link and bind vertex attributes for a shader program.
  //
  // |vertex_source| holds the source code for the vertex shader.
  //
  // |fragment_source| holds the source code for the fragment shader.
  //
  // |vertex_description| tells the shader program which vertex attributes are
  // available and should be bound before the program is linked. The format must
  // match the geometry object that is active together with this shader.
  // See GLGeometry::Create() for a description.
  static std::unique_ptr<GLShader> Create(const char* vertex_source,
                                          const char* fragment_source,
                                          const char* vertex_description);

  // Invalidate all internal GL resources. This should be used if the GL context
  // was lost. This object needs to be re-created before it can be used for
  // rendering again.
  void OnGLContextLost();

  // Sets this shader program as active in the current GL context. This needs
  // to be done before any geometry is rendered.
  void Activate();

  // Update the uniform variables used in the shader program.
  void SetUniform(const std::string& name, float s);
  void SetUniform(const std::string& name, const gfx::Vector2dF& v);
  void SetUniform(const std::string& name, const gfx::Vector3dF& v);
  void SetUniform(const std::string& name, const gfx::Vector4dF& v);
  void SetUniform(const std::string& name, int i);

 private:
  typedef std::map<std::string, GLuint> UniformMap;

  GLuint id_;
  UniformMap uniforms_;

  GLShader();

  // Clean up the internal GL resources.
  void Destroy();

  bool CreateProgram(const char* name, const char* vertex_description);
  bool CreateProgram(const char* vertex_source,
                     const char* fragment_source,
                     const char* vertex_description);
  GLuint CreateShader(const char* source, GLenum type);
  bool BindAttributeLocation(const char* vertex_description);
  GLint GetUniformLocation(const std::string& name);

  DISALLOW_COPY_AND_ASSIGN(GLShader);
};

}  // namespace opera

#endif  // COMMON_OPENGL_UTILS_GL_SHADER_H_
