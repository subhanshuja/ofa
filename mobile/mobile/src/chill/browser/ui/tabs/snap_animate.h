// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_UI_TABS_SNAP_ANIMATE_H_
#define CHILL_BROWSER_UI_TABS_SNAP_ANIMATE_H_

#include <cmath>

#include "base/time/time.h"

#include "chill/browser/ui/tabs/interpolator.h"

namespace opera {
namespace tabui {

// Ease-in-ease-out animation to move a current position to a desired target
// position.
class SnapAnimate {
 public:
  static constexpr float kDefaultDurationMs = 200;
  static base::TimeDelta DefaultDuration() {
    return base::TimeDelta::FromMilliseconds(kDefaultDurationMs);
  }

  SnapAnimate() : active_(false) { interpolator_.Reset(0.0f); }

  // Find the closest snap position and set up an animation for the scroll
  // position to get there.
  void Setup(int closest_index,
             float delta,
             float current_position,
             const base::Time& current_time,
             const base::TimeDelta duration =
                 base::TimeDelta::FromMilliseconds(kDefaultDurationMs));

  // Set up an animation to snap animate to a specific tab.
  void SetupToPosition(float snap_position,
                       float current_position,
                       const base::Time& current_time,
                       const base::TimeDelta duration =
                       base::TimeDelta::FromMilliseconds(kDefaultDurationMs));

  // Animate the scroll position until we get to the target.
  float Animate(const base::Time& current_time);

  // Disable all animation and update the current position.
  // The current position is used in Setup to check if the calculation can be
  // ignored.
  void Reset(float position);

  bool active() const { return active_; }
  float current_value() const { return interpolator_.current_value(); }

 private:
  // True if the scroll position is trying to center on a tab after input has
  // been lifted.
  bool active_;

  // The snap animation if it's active.
  Interpolator<float, true> interpolator_;

  void Setup(float delta,
             float position,
             const base::Time& current_time,
             const base::TimeDelta duration);
};

}  // namespace tabui
}  // namespace opera

#endif  // CHILL_BROWSER_UI_TABS_SNAP_ANIMATE_H_
