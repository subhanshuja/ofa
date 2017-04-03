// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/opengl_utils/gl_shader.h"

#include <cstdlib>
#include <cstring>
#include <string>

#include "base/logging.h"

namespace opera {

GLShader::GLShader() : id_(0) {}

GLShader::~GLShader() {
  Destroy();
}

std::unique_ptr<GLShader> GLShader::Create(const char* vertex_source,
                                           const char* fragment_source,
                                           const char* vertex_description) {
  std::unique_ptr<GLShader> shader(new GLShader);
  if (!shader->CreateProgram(vertex_source, fragment_source,
                             vertex_description))
    shader.reset();
  return shader;
}

void GLShader::Destroy() {
  if (id_) {
    glDeleteProgram(id_);
    id_ = 0;
  }
  uniforms_.clear();
}

void GLShader::OnGLContextLost() {
  id_ = 0;
}

void GLShader::Activate() {
  glUseProgram(id_);
}

void GLShader::SetUniform(const std::string& name, float s) {
  // Update the value if it's valid.
  GLint index = GetUniformLocation(name);
  if (index >= 0)
    glUniform1f(index, s);
}

void GLShader::SetUniform(const std::string& name, const gfx::Vector2dF& v) {
  GLint index = GetUniformLocation(name);
  if (index >= 0)
    glUniform2f(index, v.x(), v.y());
}

void GLShader::SetUniform(const std::string& name, const gfx::Vector3dF& v) {
  GLint index = GetUniformLocation(name);
  if (index >= 0)
    glUniform3f(index, v.x(), v.y(), v.z());
}

void GLShader::SetUniform(const std::string& name, const gfx::Vector4dF& v) {
  GLint index = GetUniformLocation(name);
  if (index >= 0)
    glUniform4f(index, v.x(), v.y(), v.z(), v.w());
}

void GLShader::SetUniform(const std::string& name, int i) {
  GLint index = GetUniformLocation(name);
  if (index >= 0)
    glUniform1i(index, i);
}

bool GLShader::CreateProgram(const char* vertex_source,
                             const char* fragment_source,
                             const char* vertex_description) {
  GLuint vertex_shader = CreateShader(vertex_source, GL_VERTEX_SHADER);
  if (!vertex_shader)
    return false;

  GLuint fragment_shader = CreateShader(fragment_source, GL_FRAGMENT_SHADER);
  if (!fragment_shader)
    return false;

  id_ = glCreateProgram();
  if (id_) {
    glAttachShader(id_, vertex_shader);
    glAttachShader(id_, fragment_shader);
    if (!BindAttributeLocation(vertex_description))
      return false;

    glLinkProgram(id_);
    GLint link_status = GL_FALSE;
    glGetProgramiv(id_, GL_LINK_STATUS, &link_status);
    if (link_status != GL_TRUE) {
      GLint length = 0;
      glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &length);
      if (length > 0) {
        char* buffer = reinterpret_cast<char*>(malloc(length));
        if (buffer) {
          glGetProgramInfoLog(id_, length, NULL, buffer);
          LOG(ERROR) << "Could not link program:";
          LOG(ERROR) << buffer;
          free(buffer);
        }
      }
      glDeleteProgram(id_);
      id_ = 0;
      return false;
    }
  }

  return true;
}

GLuint GLShader::CreateShader(const char* source, GLenum type) {
  GLuint shader = glCreateShader(type);
  if (!shader)
    return 0;

  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);
  GLint compiled = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    if (length) {
      char* buffer = reinterpret_cast<char*>(malloc(length));
      if (buffer) {
        glGetShaderInfoLog(shader, length, NULL, buffer);
        LOG(ERROR) << "Could not compile shader [" << type << "]:";
        LOG(ERROR) << buffer;
        free(buffer);
      }
    }
    glDeleteShader(shader);
    shader = 0;
  }

  return shader;
}

bool GLShader::BindAttributeLocation(const char* vertex_description) {
  int current = 0;
  int tex_coord = 0;

  // Parse the description.
  const size_t kBufferSize = 32;
  DCHECK_LT(strlen(vertex_description), kBufferSize);
  const char kLayoutDelimiters[] = ";/ \t";
  char buffer[kBufferSize];
  snprintf(buffer, kBufferSize, "%s", vertex_description);
  char* save_ptr;
  char* token = strtok_r(buffer, kLayoutDelimiters, &save_ptr);

  char tex_coord_buffer[kBufferSize];

  // Parse each encountered attribute.
  while (token) {
    // Check for invalid format.
    if (strlen(token) != 3)
      return false;

    switch (token[0]) {
      case 'c':
        glBindAttribLocation(id_, current++, "a_color");
        break;
      case 'n':
        glBindAttribLocation(id_, current++, "a_normal");
        break;
      case 'p':
        glBindAttribLocation(id_, current++, "a_position");
        break;

      case 't':
        snprintf(tex_coord_buffer, kBufferSize, "a_tex_coord%d", tex_coord++);
        glBindAttribLocation(id_, current++, tex_coord_buffer);
        break;

      default:
        LOG(ERROR) << "Unknown attribute: " << token;
        return false;
    }

    token = strtok_r(NULL, kLayoutDelimiters, &save_ptr);
  }

  // We need at least one position attribute.
  return current > 0;
}

GLint GLShader::GetUniformLocation(const std::string& name) {
  // Check if we've encountered this uniform before.
  UniformMap::iterator i = uniforms_.find(name);
  GLint index;
  if (i != uniforms_.end()) {
    // Yes, we already have the mapping.
    index = i->second;
  } else {
    // No, ask the driver for the mapping and save it.
    index = glGetUniformLocation(id_, name.c_str());
    uniforms_[name] = index;
  }
  return index;
}

}  // namespace opera
