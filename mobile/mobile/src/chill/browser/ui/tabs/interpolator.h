// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_UI_TABS_INTERPOLATOR_H_
#define CHILL_BROWSER_UI_TABS_INTERPOLATOR_H_

#include <cmath>

#include "base/time/time.h"

#include "cc/base/math_util.h"
#include "ui/android/animation_utils.h"
#include "ui/gfx/geometry/vector2d_f.h"

// We want to use the Interpolator for the Vector classes but they specifically
// disallow 'operator *'. Get around that with template specialization.
namespace ui {
inline gfx::Vector2dF Lerp(gfx::Vector2dF a, gfx::Vector2dF b, float t) {
  return ScaleVector2d(b - a, t) + a;
}
}  // namespace ui

namespace opera {
namespace tabui {

// Interpolate smoothly between two input values based on a third one that
// should be between the first two. The return value is clamped between 0 and 1.
// It's implemented as Cubic Hermite Interpolation after doing a clamp.
template <typename T>
inline T SmoothStep(T edge0, T edge1, T x) {
  // Scale, bias and saturate x to 0..1 range.
  x = cc::MathUtil::ClampToRange((x - edge0) / (edge1 - edge0), T(0), T(1));
  // Evaluate polynomial.
  return x * x * (T(3) - T(2) * x);
}

// Interpolate between two values over a fixed time length.
// The value is always clamped between the start and end values.
template <typename T, bool SMOOTHSTEP = false>
class Interpolator {
 public:
  Interpolator() {}

  // Reset the animation and let it idle at the given value.
  void Reset(const T& value) {
    Init(value, value, base::Time(), base::TimeDelta());
  }

  // Start off a new animation.
  void Init(const T& start_value,
            const T& end_value,
            const base::Time& start_time,
            const base::TimeDelta duration) {
    start_value_ = start_value;
    end_value_ = end_value;
    start_time_ = start_time;
    end_time_ = start_time + duration;
    current_value_ = start_value;
  }

  // Restart the animation but use the current value as the new start value.
  void InitFromCurrent(const T& end_value,
                       float start_time,
                       const base::TimeDelta duration) {
    Init(current_value_, end_value, start_time, duration);
  }

  // Continue the animation.
  // Will return true once it's finished.
  bool Update(const base::Time& current_time) {
    // Check if the animation is done.
    if (current_time > end_time_) {
      current_value_ = end_value_;
      return true;
    }

    // Guard against time travelling.
    if (current_time < start_time_) {
      current_value_ = start_value_;
    } else {
      // Figure out where we are in the time line.
      float t = (current_time - start_time_).InSecondsF() /
                (end_time_ - start_time_).InSecondsF();

      // Optionally ease-in-ease-out.
      if (SMOOTHSTEP)
        t = SmoothStep(0.0f, 1.0f, t);

      // Linearly interpolate to get the new value.
      current_value_ = ui::Lerp(start_value_, end_value_, t);
    }

    // Animation isn't done yet.
    return false;
  }

  const T& current_value() const { return current_value_; }
  const T& end_value() const { return end_value_; }

 private:
  T start_value_;
  T end_value_;
  base::Time start_time_;
  base::Time end_time_;
  T current_value_;
};

}  // namespace tabui
}  // namespace opera

#endif  // CHILL_BROWSER_UI_TABS_INTERPOLATOR_H_
