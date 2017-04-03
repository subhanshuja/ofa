// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
// Update this interface when original header/struct is updated.
#include "gpu/config/gpu_info.h"
%}

namespace gpu {

struct GPUInfo {
  // The amount of time taken to get from the process starting to the message
  // loop being pumped.
  base::TimeDelta initialization_time;

  // Computer has NVIDIA Optimus
  bool optimus;

  // Computer has AMD Dynamic Switchable Graphics
  bool amd_switchable;

  // Lenovo dCute is installed. http://crbug.com/181665.
  bool lenovo_dcute;

  // On Windows, the unique identifier of the adapter the GPU process uses.
  // The default is zero, which makes the browser process create its D3D device
  // on the primary adapter. Note that the primary adapter can change at any
  // time so it is better to specify a particular LUID. Note that valid LUIDs
  // are always non-zero.
  uint64_t adapter_luid;

  // The vendor of the graphics driver currently installed.
  std::string driver_vendor;

  // The version of the graphics driver currently installed.
  std::string driver_version;

  // The date of the graphics driver currently installed.
  std::string driver_date;

  // The version of the pixel/fragment shader used by the gpu.
  std::string pixel_shader_version;

  // The version of the vertex shader used by the gpu.
  std::string vertex_shader_version;

  // The machine model identifier with format "name major.minor".
  // Name should not contain any whitespaces.
  std::string machine_model_name;
  std::string machine_model_version;

  // The version of OpenGL we are using.
  // TODO(zmo): should be able to tell if it's GL or GLES.
  std::string gl_version;

  // The GL_VENDOR string.  "" if we are not using OpenGL.
  std::string gl_vendor;

  // The GL_RENDERER string.  "" if we are not using OpenGL.
  std::string gl_renderer;

  // The GL_EXTENSIONS string.  "" if we are not using OpenGL.
  std::string gl_extensions;

  // GL window system binding vendor.  "" if not available.
  std::string gl_ws_vendor;

  // GL window system binding version.  "" if not available.
  std::string gl_ws_version;

  // GL window system binding extensions.  "" if not available.
  std::string gl_ws_extensions;

  bool software_rendering;

  // Whether the gpu process is running in a sandbox.
  bool sandboxed;
};

}  // namespace gpu
