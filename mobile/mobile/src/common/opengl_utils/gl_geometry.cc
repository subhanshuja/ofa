// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/opengl_utils/gl_geometry.h"

#include <cstdint>
#include <cstring>

#include "base/logging.h"

#include "common/opengl_utils/gl_library.h"

namespace opera {

namespace {

// Used to parse the vertex layout,
// e.g. "p3f;c4b" for "position 3 floats, color 4 bytes".
const char kLayoutDelimiter[] = ";/ \t";

GLuint GetVertexSize(const char* vertex_description) {
  GLuint size = 0;

  // Parse the description.
  const size_t kBufferSize = 32;
  DCHECK_LT(strlen(vertex_description), kBufferSize);
  char buffer[kBufferSize];
  snprintf(buffer, kBufferSize, "%s", vertex_description);
  char* save_ptr;
  char* token = strtok_r(buffer, kLayoutDelimiter, &save_ptr);

  // Parse each encountered attribute.
  while (token) {
    // Don't care about the kind of attribute here.
    // Ignore(token[0]);

    // There can be between 1 and 4 elements in an attribute.
    size_t num_elements = token[1] - '1' + 1;
    if (num_elements < 1 || num_elements > 4)
      return 0;

    // The data type is needed, the most common ones are supported.
    size_t type_size;
    switch (token[2]) {
      case 'b':
        type_size = sizeof(GLbyte);
        break;
      case 'f':
        type_size = sizeof(GLfloat);
        break;
      case 'i':
        type_size = sizeof(GLint);
        break;
      case 's':
        type_size = sizeof(GLshort);
        break;
      case 'u':
        type_size = sizeof(GLuint);
        break;
      case 'w':
        type_size = sizeof(GLushort);
        break;
      default:
        return 0;
    }

    size += num_elements * type_size;

    token = strtok_r(NULL, kLayoutDelimiter, &save_ptr);
  }

  return size;
}

// Map the OpenGL enumerator to the actual byte size.
unsigned GetIndexSize(GLenum type) {
  switch (type) {
    case GL_UNSIGNED_BYTE:
      return sizeof(GLbyte);
    case GL_UNSIGNED_SHORT:
      return sizeof(GLushort);
    case GL_UNSIGNED_INT:
      return sizeof(GLuint);
    default:
      return 0;
  }
}

}  // namespace

GLGeometry::GLGeometry()
    : num_vertices_(0),
      vertex_size_(0),
      vertex_array_id_(0),
      vertex_buffer_id_(0),
      num_indices_(0),
      primitive_(GL_NONE),
      index_type_(GL_NONE),
      index_buffer_id_(0) {}

GLGeometry::~GLGeometry() {
  Destroy();
}

std::unique_ptr<GLGeometry> GLGeometry::Create(GLenum primitive,
                                               const char* vertex_description,
                                               unsigned num_vertices,
                                               const void* vertices,
                                               GLenum index_description,
                                               unsigned num_indices,
                                               const void* indices) {
  std::unique_ptr<GLGeometry> geometry(new GLGeometry);
  if (!geometry->Setup(primitive, vertex_description, num_vertices, vertices,
                       index_description, num_indices, indices))
    geometry.reset();
  return geometry;
}

bool GLGeometry::Setup(GLenum primitive,
                       const char* vertex_description,
                       unsigned num_vertices,
                       const void* vertices,
                       GLenum index_description,
                       unsigned num_indices,
                       const void* indices) {
  Destroy();

  // Verify that we have a valid layout and get the total byte size per vertex.
  vertex_size_ = GetVertexSize(vertex_description);
  if (!vertex_size_) {
    LOG(ERROR) << "Invalid vertex layout.";
    return false;
  }

  num_vertices_ = num_vertices;
  num_indices_ = num_indices;
  primitive_ = primitive;

  if (GLLibrary::Get().SupportsVAO()) {
    glGenVertexArraysOES(1, &vertex_array_id_);
    glBindVertexArrayOES(vertex_array_id_);
  }

  // Create the vertex buffer and upload the data.
  glGenBuffers(1, &vertex_buffer_id_);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_id_);
  glBufferData(GL_ARRAY_BUFFER, num_vertices * vertex_size_, vertices,
               GetVertexBufferType());

  // Make sure the vertex format is understood and the attribute pointers are
  // set up.
  bool result = SetupVertexLayout(vertex_description, vertex_size_);

  // Create the index buffer and upload the data.
  if (result && indices) {
    index_type_ = index_description;

    glGenBuffers(1, &index_buffer_id_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_id_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 num_indices * GetIndexSize(index_type_), indices,
                 GL_STATIC_DRAW);
  }

  if (vertex_array_id_) {
    // De-activate the buffer again and we're done.
    glBindVertexArrayOES(0);
  } else {
    // De-activate the individual buffers.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }

  return result;
}

void GLGeometry::Destroy() {
  if (vertex_array_id_)
    glBindVertexArrayOES(vertex_array_id_);
  if (index_buffer_id_) {
    glDeleteBuffers(1, &index_buffer_id_);
    index_buffer_id_ = 0;
  }
  if (vertex_buffer_id_) {
    glDeleteBuffers(1, &vertex_buffer_id_);
    vertex_buffer_id_ = 0;
  }
  if (vertex_array_id_) {
    glBindVertexArrayOES(0);
    glDeleteVertexArraysOES(1, &vertex_array_id_);
    vertex_array_id_ = 0;
  }
  vertex_layout_.clear();
}

void GLGeometry::OnGLContextLost() {
  // All resources are invalid.
  index_buffer_id_ = 0;
  vertex_buffer_id_ = 0;
  vertex_array_id_ = 0;
}

void GLGeometry::Draw() {
  // Set up the vertex data.
  if (vertex_array_id_) {
    glBindVertexArrayOES(vertex_array_id_);
  } else if (vertex_buffer_id_) {
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_id_);
    for (GLuint attribute_index = 0;
         attribute_index < (GLuint)vertex_layout_.size(); ++attribute_index) {
      Element& e = vertex_layout_[attribute_index];
      glVertexAttribPointer(attribute_index, e.num_elements_, e.type_, GL_FALSE,
                            vertex_size_,
                            reinterpret_cast<const GLvoid*>(e.vertex_offset_));
      glEnableVertexAttribArray(attribute_index);
    }

    if (index_buffer_id_ && (num_indices_ > 0))
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_id_);
  }

  // Draw the primitive.
  if (index_buffer_id_ && (num_indices_ > 0))
    glDrawElements(primitive_, num_indices_, index_type_, NULL);
  else
    glDrawArrays(primitive_, 0, num_vertices_);

  // Clean up states.
  if (vertex_array_id_) {
    glBindVertexArrayOES(0);
  } else {
    for (GLuint attribute_index = 0;
         attribute_index < (GLuint)vertex_layout_.size(); ++attribute_index) {
      glDisableVertexAttribArray(attribute_index);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  }
}

bool GLGeometry::SetupVertexLayout(const char* vertex_description,
                                   GLuint vertex_size) {
  // Parse the layout.
  const size_t kBufferSize = 32;
  DCHECK_LT(strlen(vertex_description), kBufferSize);
  char buffer[kBufferSize];
  snprintf(buffer, kBufferSize, "%s", vertex_description);
  char* save_ptr;
  char* token = strtok_r(buffer, kLayoutDelimiter, &save_ptr);

  // OpenGL ES 2.0 has a minimum requirement of 8 and a maximum of 16.
  const int kMaxAttributes = 8;

  // Parse each encountered attribute.
  GLuint attribute_index = 0;
  size_t vertex_offset = 0;
  while (token) {
    // Check for invalid format.
    if (strlen(token) != 3)
      return false;

    if (attribute_index >= kMaxAttributes)
      return false;

    // Don't care about the kind of attribute here.
    // Ignore(token[0]);

    // There can be between 1 and 4 elements in an attribute.
    GLsizei num_elements = token[1] - '1' + 1;
    if (num_elements < 1 || num_elements > 4)
      return false;

    // The data type is needed, the most common ones are supported.
    GLenum type;
    size_t type_size;
    switch (token[2]) {
      case 'b':
        type = GL_UNSIGNED_BYTE;
        type_size = sizeof(GLbyte);
        break;
      case 'f':
        type = GL_FLOAT;
        type_size = sizeof(GLfloat);
        break;
      case 's':
        type = GL_SHORT;
        type_size = sizeof(GLshort);
        break;
      case 'w':
        type = GL_UNSIGNED_SHORT;
        type_size = sizeof(GLushort);
        break;
      default:
        return false;
    }

    // We got all we need to define this attribute.
    if (vertex_array_id_) {
      // This will be saved into the vertex array object.
      glVertexAttribPointer(attribute_index, num_elements, type, GL_FALSE,
                            vertex_size,
                            reinterpret_cast<const GLvoid*>(vertex_offset));
      glEnableVertexAttribArray(attribute_index);
    } else {
      // Need to keep this information for when rendering.
      Element element;
      element.num_elements_ = num_elements;
      element.type_ = type;
      element.vertex_offset_ = vertex_offset;
      vertex_layout_.push_back(element);
    }

    // Move on to the next attribute.
    ++attribute_index;
    vertex_offset += num_elements * type_size;
    token = strtok_r(NULL, kLayoutDelimiter, &save_ptr);
  }

  // Make sure we don't leave any attribute arrays from any previous calls
  // enabled.
  while (attribute_index < kMaxAttributes)
    glDisableVertexAttribArray(attribute_index++);

  return true;
}

std::unique_ptr<GLDynamicGeometry> GLDynamicGeometry::Create(
    GLenum primitive,
    const char* vertex_description,
    unsigned num_vertices,
    const void* vertices,
    GLenum index_description,
    unsigned num_indices,
    const void* indices) {
  std::unique_ptr<GLDynamicGeometry> geometry(new GLDynamicGeometry);
  if (!geometry->Setup(primitive, vertex_description, num_vertices, vertices,
                       index_description, num_indices, indices))
    geometry.reset();
  return geometry;
}

void GLDynamicGeometry::Update(const void* vertices) {
  if (vertex_buffer_id_) {
    if (vertex_array_id_)
      glBindVertexArrayOES(vertex_array_id_);

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_id_);
    glBufferData(GL_ARRAY_BUFFER, num_vertices_ * vertex_size_, vertices,
                 GL_DYNAMIC_DRAW);

    if (vertex_array_id_)
      glBindVertexArrayOES(0);
  }
}

}  // namespace opera
