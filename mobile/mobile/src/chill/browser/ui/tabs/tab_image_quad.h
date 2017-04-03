// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_UI_TABS_TAB_IMAGE_QUAD_H_
#define CHILL_BROWSER_UI_TABS_TAB_IMAGE_QUAD_H_

#include "ui/gfx/geometry/vector2d_f.h"

#include "chill/browser/ui/tabs/image_quad.h"
#include "common/opengl_utils/vector4d_f.h"

namespace opera {

class GLMipMap;

namespace tabui {

class TabSelectorRenderer;

// An ImageQuad extended with methods suitable for drawing a tab chrome or
// thumbnail.
class TabImageQuad : public ImageQuad {
 public:
  TabImageQuad();

  void Draw(const gfx::Vector2dF& offset,
            const gfx::Vector2dF& scale,
            const gfx::Vector4dF& color,
            const gfx::Vector2dF& cropping,
            float dimmer,
            TabSelectorRenderer* renderer);

  bool IsVisible(const gfx::Vector2dF& offset,
                 const gfx::Vector2dF& scale,
                 const gfx::Vector2dF& projection_scale) const;
};

}  // namespace tabui
}  // namespace opera

#endif  // CHILL_BROWSER_UI_TABS_TAB_IMAGE_QUAD_H_
