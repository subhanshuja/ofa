// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/opengl_utils/gl_library.h"

#include <dlfcn.h>
#include <sstream>

#include "base/logging.h"
#include "ui/gl/gl_version_info.h"

#include "common/opengl_utils/opengl.h"

// Global function pointers. These will be null unless the GLLibrary object has
// been initialized and the extension is verified to be available.
PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOES;
PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOES;
PFNGLGENVERTEXARRAYSOESPROC glGenVertexArraysOES;

namespace opera {

class DynamicLibrary {
 public:
  explicit DynamicLibrary(const char* name) {
    library_ = dlopen(name, RTLD_LOCAL | RTLD_LAZY);
    if (!library_)
      LOG(ERROR) << "LoadLibrary failed: " << name;
  }

  ~DynamicLibrary() {
    if (library_)
      dlclose(library_);
  }

  operator void*() { return library_; }

 private:
  void* library_;
};

DynamicLibrary g_libEGL("libEGL.so");
DynamicLibrary g_libGLESv2("libGLESv2.so");

// Try to get an OpenGL related entry point.
void* GetProcAddress(const char* name) {
  if ((name[0] == 'e') && (name[1] == 'g') && (name[2] == 'l') && g_libEGL)
    return dlsym(g_libEGL, name);

  if ((name[0] == 'g') && (name[1] == 'l') && g_libGLESv2)
    return dlsym(g_libGLESv2, name);

  return NULL;
}

GLLibrary& GLLibrary::Get() {
  static GLLibrary gl;
  return gl;
}

GLLibrary::GLLibrary()
    : initialized_(false),
      npot_(false),
      vertex_array_objects_(false),
      mipmapping_(true),
      async_upload_(true),
      shader_texture_lod_(false) {}

bool GLLibrary::Init() {
  if (initialized_)
    return true;

  // Get information about the currently active context.
  const char* renderer =
      reinterpret_cast<const char*>(glGetString(GL_RENDERER));
  const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));

  VLOG(2) << "OpenGL:";
  VLOG(2) << "  vendor:         "
          << reinterpret_cast<const char*>(glGetString(GL_VENDOR));
  VLOG(2) << "  renderer:       " << renderer;
  VLOG(2) << "  version:        " << version;
  VLOG(2) << "  shader version: " << reinterpret_cast<const char*>(glGetString(
                                         GL_SHADING_LANGUAGE_VERSION));

  // Extract the version since some extensions are included by default in newer
  // versions.
  unsigned major_version;
  unsigned minor_version;
  bool is_es;
  bool is_es2;
  bool is_es3;
  gl::GLVersionInfo::ParseVersionString(version, &major_version, &minor_version,
                                        &is_es, &is_es2, &is_es3);
  if (major_version < 2) {
    LOG(ERROR) << "Unsupported OpenGL ES version.";
    return false;
  }

  // Log out the extensions.
  std::stringstream stream(
      reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)));
  std::string token;
  std::set<std::string> extensions;
  while (std::getline(stream, token, ' '))
    extensions.insert(token);

  // Load regular OpenGL extensions we want.
  if (extensions.count("GL_OES_vertex_array_object")) {
    glBindVertexArrayOES = reinterpret_cast<PFNGLBINDVERTEXARRAYOESPROC>(
        GetProcAddress("glBindVertexArrayOES"));
    glDeleteVertexArraysOES = reinterpret_cast<PFNGLDELETEVERTEXARRAYSOESPROC>(
        GetProcAddress("glDeleteVertexArraysOES"));
    glGenVertexArraysOES = reinterpret_cast<PFNGLGENVERTEXARRAYSOESPROC>(
        GetProcAddress("glGenVertexArraysOES"));

    if (glBindVertexArrayOES && glDeleteVertexArraysOES &&
        glGenVertexArraysOES) {
      // This extension seems to be broken on older PowerVR drivers.
      if (!strstr(renderer, "PowerVR SGX 53") &&
          !strstr(renderer, "PowerVR SGX 54")) {
        vertex_array_objects_ = true;
      }
    }
  }
  if (extensions.count("GL_ARB_texture_non_power_of_two") ||
      extensions.count("GL_OES_texture_npot") || is_es3) {
    npot_ = true;
  }

  if (extensions.count("GL_EXT_shader_texture_lod"))
    shader_texture_lod_ = true;

  // Check for supported texture compression extensions.
  if (extensions.count("GL_AMD_compressed_ATC_texture") ||
      extensions.count("GL_ATI_texture_compression_atitc"))
    texture_compression_.atc_ = true;
  if (extensions.count("GL_EXT_texture_compression_dxt1"))
    texture_compression_.dxt1_ = true;
  if (extensions.count("GL_OES_compressed_ETC1_RGB8_texture"))
    texture_compression_.etc1_ = true;
  if (extensions.count("GL_EXT_texture_compression_s3tc"))
    texture_compression_.s3tc_ = true;

  // Check for known broken drivers.
  if (strstr(renderer, "PowerVR SGX 53") ||
      strstr(renderer, "PowerVR SGX 54")) {
    mipmapping_ = false;
  }

  if (strstr(renderer, "Adreno (TM) 32") ||
      strstr(renderer, "PowerVR SGX 53") ||
      strstr(renderer, "PowerVR SGX 54") ||
      strstr(renderer, "Vivante GC4000")) {
    async_upload_ = false;
  }

  // Log driver information.
  VLOG(2) << "  extensions:";
  for (const auto& extension : extensions)
    VLOG(2) << "    " << extension.c_str();

  if (vertex_array_objects_)
    VLOG(2) << "Supports Vertex Array Objects.";
  if (npot_)
    VLOG(2) << "Supports NPOT.";
  if (shader_texture_lod_)
    VLOG(2) << "Supports LOD.";
  if (!mipmapping_)
    VLOG(2) << "Mipmapping not supported.";
  if (!async_upload_)
    VLOG(2) << "Asynchronous uploading not supported.";

  VLOG(2) << "TextureCompression:";
  VLOG(2) << "  atc:   " << texture_compression_.atc_;
  VLOG(2) << "  dxt1:  " << texture_compression_.dxt1_;
  VLOG(2) << "  etc1:  " << texture_compression_.etc1_;
  VLOG(2) << "  s3tc:  " << texture_compression_.s3tc_;

  initialized_ = true;
  return true;
}

void GLLibrary::Viewport(int width, int height) {
  glViewport(0, 0, width, height);
}

void GLLibrary::Clear(const float rgba[4]) {
  glClearColor(rgba[0], rgba[1], rgba[2], rgba[3]);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLLibrary::EnableBlend() {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

}  // namespace opera
