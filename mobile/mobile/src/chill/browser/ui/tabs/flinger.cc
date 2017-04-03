// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/ui/tabs/flinger.h"

#include <cmath>

#include "base/logging.h"

namespace opera {
namespace tabui {

namespace {

static constexpr float kGateThreshold = 0.01f;
static constexpr float kMomentum = 80.f;

base::TimeDelta GetDurationFromSpeed(float velocity_pps) {
  float abs_velocity_ppms = std::abs(velocity_pps / 1000.f);
  if (abs_velocity_ppms <= kGateThreshold) {
    return base::TimeDelta();
  }
  float dur_ms = -log(kGateThreshold / abs_velocity_ppms) * kMomentum;
  return base::TimeDelta::FromMilliseconds(dur_ms);
}

// NOTE: Return value is pps.
float GetSpeedFromDistance(float distance_px) {
  float speed_ppms = distance_px / kMomentum;
  if (distance_px > kGateThreshold) {
    speed_ppms += kGateThreshold;
  } else if (distance_px < -kGateThreshold) {
    speed_ppms -= kGateThreshold;
  }
  float speed_pps = speed_ppms * 1000.f;
  return speed_pps;
}

float GetDistanceAtTime(float start_velocity_pps,
                        const base::TimeDelta& elapsed_time) {
  float elapsed_time_ms = static_cast<float>(elapsed_time.InMillisecondsF());
  float start_velocity_ppms = start_velocity_pps / 1000.f;
  DCHECK(elapsed_time_ms >= 0.f);
  float dist_px = (1.f - std::exp(-elapsed_time_ms / kMomentum)) * kMomentum *
                  start_velocity_ppms;
  return dist_px;
}
}  // namespace

Flinger::Flinger()
    : start_velocity_pps_(0.f), state_distance_(0.f), is_flinging_(false) {}

void Flinger::Start(const base::Time& current_time, float velocity_pps) {
  state_distance_ = 0.f;
  start_velocity_pps_ = velocity_pps;
  start_time_ = current_time;
  end_time_ = start_time_ + GetDurationFromSpeed(velocity_pps);
  is_flinging_ = end_time_ > start_time_;
}

void Flinger::Adjust(float delta) {
  float distance =
      GetDistanceAtTime(start_velocity_pps_, end_time_ - start_time_);
  distance -= delta;
  start_velocity_pps_ = GetSpeedFromDistance(distance);
  end_time_ = start_time_ + GetDurationFromSpeed(start_velocity_pps_);
}

void Flinger::Cancel() {
  start_velocity_pps_ = 0.f;
  state_distance_ = 0.f;
  end_time_ = start_time_;
  is_flinging_ = false;
}

float Flinger::GetEndDistance() const {
  return GetDistanceAtTime(start_velocity_pps_, end_time_ - start_time_);
}

float Flinger::GetDistance(const base::Time& current_time) {
  base::TimeDelta elapsed_time;
  is_flinging_ = current_time < end_time_;
  elapsed_time = (is_flinging_ ? current_time : end_time_) - start_time_;
  return GetDistanceAtTime(start_velocity_pps_, elapsed_time);
}

float Flinger::GetDelta(const base::Time& current_time) {
  float distance = GetDistance(current_time);
  float delta_px = distance - state_distance_;
  state_distance_ = distance;
  return delta_px;
}

}  // namespace tabui
}  // namespace opera
