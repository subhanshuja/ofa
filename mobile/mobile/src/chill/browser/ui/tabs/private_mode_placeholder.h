// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_UI_TABS_PRIVATE_MODE_PLACEHOLDER_H_
#define CHILL_BROWSER_UI_TABS_PRIVATE_MODE_PLACEHOLDER_H_

#include <jni.h>

#include <memory>

#include "ui/gfx/geometry/vector2d_f.h"

#include "chill/browser/ui/tabs/image_quad.h"

namespace opera {

class GLTexture;

namespace tabui {

class TabSelectorRenderer;

class PrivateModePlaceholder : public ImageQuad {
 public:
  PrivateModePlaceholder();

  void OnCreate(jobject bitmap, TabSelectorRenderer* renderer);
  void OnSurfaceChanged(int width, int height);

  bool IsVisible(float position) const;
  void Draw(float position, float dimmer, TabSelectorRenderer* renderer);

 private:
  float mipmap_ratio_;
  gfx::Vector2dF scale_;

  void AdjustScale(int screen_w, int screen_h);
};

}  // namespace tabui
}  // namespace opera

#endif  // CHILL_BROWSER_UI_TABS_PRIVATE_MODE_PLACEHOLDER_H_
