// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OPENGL_UTILS_VECTOR4D_F_H_
#define COMMON_OPENGL_UTILS_VECTOR4D_F_H_

namespace gfx {

class Vector4dF {
 public:
  Vector4dF() : x_(0), y_(0), z_(0), w_(0) {}
  Vector4dF(float x, float y, float z, float w) : x_(x), y_(y), z_(z), w_(w) {}

  float x() const { return x_; }
  void set_x(float x) { x_ = x; }

  float y() const { return y_; }
  void set_y(float y) { y_ = y; }

  float z() const { return z_; }
  void set_z(float z) { z_ = z; }

  float w() const { return w_; }
  void set_w(float w) { w_ = w; }

 private:
  float x_;
  float y_;
  float z_;
  float w_;
};

}  // namespace gfx

#endif  // COMMON_OPENGL_UTILS_VECTOR4D_F_H_
