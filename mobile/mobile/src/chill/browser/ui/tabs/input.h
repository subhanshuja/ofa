// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_UI_TABS_INPUT_H_
#define CHILL_BROWSER_UI_TABS_INPUT_H_

#include "ui/gfx/geometry/vector2d_f.h"

namespace opera {
namespace tabui {
namespace input {

// 'input'            touch coordinates.
// 'min', 'max'       bounding box in view space.
// 'projection_scale' view scale in landscape mode.
//
// Input touch coordinates:  The view coordinates:
//
// 0,0                                1,1
//  +--------+               +--------+
//  |        |               |        |
//  |        |               |  +--+  |
//  |        |               |  |  |  |
//  |        |               |  |  |  |
//  |        |               |  +--+  |
//  |        |               |        |
//  |        |               |        |
//  +--------+               +--------+
//           1,1           -1,-1
//

// Check if the point is inside the bounding box.
bool Inside(gfx::Vector2dF input, gfx::Vector2dF min, gfx::Vector2dF max);

// Convert the input/tap coordinates to the view coordinate system.
gfx::Vector2dF ConvertToLocal(gfx::Vector2dF input,
                              gfx::Vector2dF projection_scale);

}  // namespace input
}  // namespace tabui
}  // namespace opera

#endif  // CHILL_BROWSER_UI_TABS_INPUT_H_
