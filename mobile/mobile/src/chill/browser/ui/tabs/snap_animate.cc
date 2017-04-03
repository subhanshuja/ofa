// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/ui/tabs/snap_animate.h"

namespace opera {
namespace tabui {

void SnapAnimate::Setup(int closest_index,
                        float delta,
                        float current_position,
                        const base::Time& current_time,
                        const base::TimeDelta duration) {
  if (closest_index != -1)
    Setup(delta, current_position, current_time, duration);
}

void SnapAnimate::SetupToPosition(float snap_position,
                                  float current_position,
                                  const base::Time& current_time,
                                  const base::TimeDelta duration) {
  float delta = snap_position + current_position;
  Setup(delta, current_position, current_time, duration);
}

void SnapAnimate::Setup(float delta,
                        float position,
                        const base::Time& current_time,
                        const base::TimeDelta duration) {
  active_ = (delta != 0);
  interpolator_.Init(position, position - delta, current_time, duration);
}

float SnapAnimate::Animate(const base::Time& current_time) {
  bool done = interpolator_.Update(current_time);
  if (done)
    active_ = false;

  return interpolator_.current_value();
}

void SnapAnimate::Reset(float position) {
  active_ = false;
  interpolator_.Reset(position);
}

}  // namespace tabui
}  // namespace opera
