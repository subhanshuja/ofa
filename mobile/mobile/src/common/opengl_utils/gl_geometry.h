// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OPENGL_UTILS_GL_GEOMETRY_H_
#define COMMON_OPENGL_UTILS_GL_GEOMETRY_H_

#include <vector>

#include "base/macros.h"
#include "common/opengl_utils/opengl.h"

namespace opera {

// A geometric primitive that can be drawn in OpenGL.
//
// It holds the primitive type together with the vertex and index resources.
// The attributes used by the vertex data needs to be specified on creation.
class GLGeometry {
  friend class GLDynamicGeometry;

 public:
  virtual ~GLGeometry();

  // Set up the internal GL resources.
  //
  // |primitive| can be one of GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES
  // GL_TRIANGLE_STRIP or GL_TRIANGLES.
  //
  // |vertex_description| is a string describing the vertex attributes in the
  // form of 'semantic', 'number of elements' and 'type' where different
  // attributes are separated by a semicolon. For example "p3f;c4b" means that
  // the vertex stream has "position using 3 floats (xyz)" followed by "color
  // using 4 bytes".
  //
  // |num_vertices| must contain at least on full primitive elements, e.g. 3 for
  // the triangle types and 2 for line types. If indices aren't used then it
  // needs to be a multiple of the primitive type's natural element length, e.g.
  // divisable with 3 for GL_TRIANGLES or 2 for GL_LINES.
  //
  // |vertices| is a tighly packed array of vertex data matching the
  // 'vertex_description'.
  //
  // |index_description| can be GL_NONE when no indices are used or describe
  // the data format type of the index array (e.g. GL_UNSIGNED_SHORT).
  //
  // |num_indices| must follow the same rules as 'num_vertices' if it's used.
  //
  // |indices| is a tighly packed array of index data matching the
  // 'index_description'.
  static std::unique_ptr<GLGeometry> Create(GLenum primitive,
                                            const char* vertex_description,
                                            unsigned num_vertices,
                                            const void* vertices,
                                            GLenum index_description = GL_NONE,
                                            unsigned num_indices = 0,
                                            const void* indices = NULL);

  // Invalidate all internal GL resources. This should be used if the GL context
  // was lost. This object needs to be re-created before it can be rendered
  // again.
  void OnGLContextLost();

  // Issue a draw command to the current GL context for this geometry.
  void Draw();

 protected:
  unsigned num_vertices_;
  size_t vertex_size_;
  GLuint vertex_array_id_;
  GLuint vertex_buffer_id_;

  bool Setup(GLenum primitive,
             const char* vertex_description,
             unsigned num_vertices,
             const void* vertices,
             GLenum index_description,
             unsigned num_indices,
             const void* indices);

 private:
  struct Element {
    GLsizei num_elements_;
    GLenum type_;
    size_t vertex_offset_;
  };

  unsigned num_indices_;
  GLenum primitive_;
  GLenum index_type_;
  std::vector<Element> vertex_layout_;

  GLuint index_buffer_id_;

  GLGeometry();

  // Clean up the internal GL resources.
  void Destroy();

  bool SetupVertexLayout(const char* vertex_description, GLuint vertex_size);

  virtual GLenum GetVertexBufferType() const { return GL_STATIC_DRAW; }

  DISALLOW_COPY_AND_ASSIGN(GLGeometry);
};

// Same as GLGeometry but also allows for dynamically updating the vertex data.
class GLDynamicGeometry : public GLGeometry {
 public:
  static std::unique_ptr<GLDynamicGeometry> Create(
      GLenum primitive,
      const char* vertex_description,
      unsigned num_vertices,
      const void* vertices,
      GLenum index_description = GL_NONE,
      unsigned num_indices = 0,
      const void* indices = NULL);
  // Replace the vertex data.
  void Update(const void* vertices);

 private:
  GLDynamicGeometry() {}

  GLenum GetVertexBufferType() const override { return GL_DYNAMIC_DRAW; }

  DISALLOW_COPY_AND_ASSIGN(GLDynamicGeometry);
};

}  // namespace opera

#endif  // COMMON_OPENGL_UTILS_GL_GEOMETRY_H_
