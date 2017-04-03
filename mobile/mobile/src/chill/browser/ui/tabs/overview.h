// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_UI_TABS_OVERVIEW_H_
#define CHILL_BROWSER_UI_TABS_OVERVIEW_H_

#include "ui/gfx/geometry/vector2d_f.h"

#include "chill/browser/ui/tabs/interpolator.h"
#include "chill/browser/ui/tabs/tab_view.h"
#include "common/opengl_utils/gl_geometry.h"
#include "common/opengl_utils/gl_shader.h"

namespace opera {
namespace tabui {

class TabSelectorRenderer;

// Miniature version of the regular tab row with the currently active one scaled
// up and highlighted. Allows for quick navigation by tapping on them.
class Overview {
 public:
  VisibleTabsContainer Cull(float scroll_position,
                            float camera_target,
                            float dimmer,
                            TabSelectorRenderer* host);

  // Check if the input event was inside the bounding box and if so returns the
  // index to the lucky one. Otherwise it returns -1.
  int CheckHit(gfx::Vector2dF input, TabSelectorRenderer* host);

 private:
  gfx::Vector2dF bounding_box_normal_min_;
  gfx::Vector2dF bounding_box_normal_max_;

  gfx::Vector2dF bounding_box_private_min_;
  gfx::Vector2dF bounding_box_private_max_;
};

}  // namespace tabui
}  // namespace opera

#endif  // CHILL_BROWSER_UI_TABS_OVERVIEW_H_
