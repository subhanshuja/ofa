// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/ui/tabs/input.h"

namespace opera {
namespace tabui {
namespace input {

bool Inside(gfx::Vector2dF input, gfx::Vector2dF min, gfx::Vector2dF max) {
  return ((input.x() >= min.x()) && (input.y() >= min.y()) &&
          (input.x() <= max.x()) && (input.y() <= max.y()));
}

gfx::Vector2dF ConvertToLocal(gfx::Vector2dF input,
                              gfx::Vector2dF projection_scale) {
  // The y axis is inverted.
  gfx::Vector2dF view(input.x(), 1.0f - input.y());

  // Convert from (0,0)..(1,1) to (-1,-1)..(1,1).
  view -= gfx::Vector2dF(0.5f, 0.5f);
  view.Scale(2.0f);

  // Extend the view rect to landscape mode is necessary.
  view.Scale(1.0f / projection_scale.x(), 1.0f / projection_scale.y());

  return view;
}

}  // namespace input
}  // namespace tabui
}  // namespace opera
