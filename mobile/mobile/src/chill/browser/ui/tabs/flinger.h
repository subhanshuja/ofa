// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_UI_TABS_MOTION_TRACKER_H_
#define CHILL_BROWSER_UI_TABS_MOTION_TRACKER_H_

#include "base/time/time.h"

namespace opera {
namespace tabui {

// Port of mini's fling implementation to C++. Can compute duration
// and end point given a start velocity, and can adjust initial
// position by changing duration and start velocity.
class Flinger {
 public:
  Flinger();
  void Start(const base::Time& current_time, float velocity_pps);
  void Adjust(float distance);
  void Cancel();
  float GetEndDistance() const;
  float GetDistance(const base::Time& current_time);
  float GetDelta(const base::Time& current_time);

  bool is_flinging() const { return is_flinging_; }

 private:
  base::Time start_time_;
  base::Time end_time_;
  float start_velocity_pps_;
  float state_distance_;
  bool is_flinging_;
};

}  // namespace tabui
}  // namespace opera

#endif  // CHILL_BROWSER_UI_TABS_MOTION_TRACKER_H_
