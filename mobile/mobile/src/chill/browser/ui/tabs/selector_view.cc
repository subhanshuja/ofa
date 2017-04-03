// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/ui/tabs/selector_view.h"

#include <algorithm>
#include <cmath>

#include "base/logging.h"

#include "chill/browser/ui/tabs/input.h"
#include "chill/browser/ui/tabs/private_mode_placeholder.h"
#include "chill/browser/ui/tabs/tab_selector_renderer.h"
#include "chill/browser/ui/tabs/tab_view.h"
#include "chill/browser/ui/tabs/upload_queue.h"
#include "common/opengl_utils/gl_mipmap.h"

namespace opera {
namespace tabui {

namespace {
// How many seconds a zoom in/out should take.
const base::TimeDelta kZoomDuration = base::TimeDelta::FromMilliseconds(100);

// How many seconds highlighting a tab should take.
const base::TimeDelta kHighlightDuration =
    base::TimeDelta::FromMilliseconds(100);

// Vertex color intensity for (un)selected tabs.
const float kSelectedIntensity = 1.0f;
const float kUnselectedIntensity = 0.75f;

// Draw scale for (un)selected tabs.
const float kSelectedScale = 0.99f;
const float kUnselectedScale = 1.0f;

// How far away vertically from the standard position does a tab need to be
// dragged before it's considered an "open" or "delete" gesture.
const float kThresholdY = 0.1f;

// Normalized units of space between each tab.
const float kSpaceBetweenTabs = 0.05f;

// The distance between the center point of each tab.
const float kTabDistance = 1.0f + kSpaceBetweenTabs;

// There should be a gap between the rows of normal and private tabs.
const float kSpaceBetweenNormalAndPrivateMode = 0.5f;

// Don't allow completely transparent chrome since will affect the size of the
// tab.
const float kMinChromeOpacity = 0.01f;

// Threshold for how far tab needs to be dragged upwards in order for
// a drop to delete it.
const float kDropDeleteThreshold = 0.5f;

// Threshold for how far tab needs to be flung upwards in order to
// delete it.
const float kFlingDeleteThreshold = 0.35f;

// Threshold for how far tab needs to be drawn downwards in order for
// a drop to open it.
const float kOpenThreshold = 0.6f;

// Duration of delete animation.
const base::TimeDelta kDeleteTime = base::TimeDelta::FromMilliseconds(200);
}  // namespace

SelectorView::SelectorView(TabSelectorRenderer* host)
    : private_mode_placeholder_(std::make_shared<PrivateModePlaceholder>()),
      scroll_position_(0),
      centered_tab_on_drag_start_(0),
      active_tab_y_(0),
      drag_active_(false),
      drag_direction_(kUnknown),
      host_(host) {
  ResetVariables();
}

SelectorView::~SelectorView() {}

bool SelectorView::OnCreate() {
  if (!background_theme_.OnCreate()) {
    LOG(ERROR) << "Failed to set up background theme.";
    return false;
  }

  return true;
}

void SelectorView::OnGLContextLost() {
  background_theme_.OnGLContextLost();
  private_mode_placeholder_->OnGLContextLost();
}

void SelectorView::OnSurfaceChanged(int width, int height) {
  private_mode_placeholder_->OnSurfaceChanged(width, height);
}

void SelectorView::OnEnter() {
  ResetVariables();
  flinger_.Cancel();

  // We want to zoom into the active tab centered.
  if (host_->tabs().size() > 0) {
    auto& active_tab = host_->active_tab();
    if (active_tab) {
      scroll_position_ = -GetFixedTabOffsetInSelector(active_tab).x();
      snap_animate_.Reset(scroll_position_);
    }

    EnterZoomOutState();
  }
}

void SelectorView::RequestUpload(const VisibleTabsContainer& big_tabs,
                                 const VisibleTabsContainer& small_tabs) {
  // Update target resolution for each tab to avoid having more mip levels
  // uploaded than what will be used when drawing.
  std::unordered_map<TabView*, TabView::VisibleSize> tab_map;

  // All tabs should at least have a minimal texture uploaded to always have
  // something to draw when they get swiped in.
  for (auto& tab : host_->tabs())
    tab_map[tab.get()] = TabView::kZeroSize;

  // Tabs that are visible as mini thumbnails.
  for (auto& entry : small_tabs)
    tab_map[entry.tab.get()] = TabView::kSmallSize;

  // Tabs that are visible as large thumbnails.
  for (auto& entry : big_tabs)
    tab_map[entry.tab.get()] = TabView::kBigSize;

  // If a tab is zoomed.
  switch (selected_state_.state()) {
    case SelectedState::kSelected:
      DCHECK(selected_state_.tab().get());
      tab_map[selected_state_.tab().get()] = TabView::kZoomedSize;
      break;

    case SelectedState::kZoomIn:
    case SelectedState::kZoomedIn:
    case SelectedState::kZoomOut:
      if (host_->active_tab().get())
        tab_map[host_->active_tab().get()] = TabView::kZoomedSize;
      break;

    default:
      break;
  }

  for (auto& p : tab_map)
    p.first->RequestUpload(p.second, host_->portrait_mode());
}

void SelectorView::OnDraw() {
  Update();

  // Normalize the snap distance and remove the space between tabs.
  // Use snap animate to get proper index when crossing the normal/private gap.
  int camera_target_index;
  float camera_target_delta = CalcSnapDelta(&camera_target_index);
  float camera_target = camera_target_index - camera_target_delta;

  // Figure out if there should be any fading between normal and private mode.

  // If there are private tabs then we know where they start, otherwise we check
  // for the placeholder tab.
  float normal_border =
      GetFixedTabOffsetInSelector(host_->public_tab_count() - 1).x();
  float private_border =
      GetFixedTabOffsetInSelector(host_->public_tab_count()).x();
  float dimmer;
  if (-scroll_position_ <= normal_border) {
    dimmer = 0.0f;
  } else if (-scroll_position_ >= private_border) {
    dimmer = 1.0f;
  } else {
    dimmer =
        (-scroll_position_ - normal_border) / (private_border - normal_border);
  }

  // Draw the background elements.
  background_theme_.Draw(dimmer, host_);

  // Draw the foreground elements.
  float tab_dimmer = 1.0f - dimmer * 2.0f;
  VisibleTabsContainer visible_tabs =
      overview_.Cull(scroll_position_, camera_target, tab_dimmer, host_);
  VisibleTabsContainer visible_big = CullTabs(tab_dimmer);

  RequestUpload(visible_big, visible_tabs);

  float placeholder_position =
      GetFixedTabOffsetInSelector(host_->tabs().size()).x() + scroll_position_;
  bool placeholder_visible =
      !host_->private_tab_count() && !host_->tabs().empty() &&
      selected_state_.state() != SelectedState::kZoomIn &&
      selected_state_.state() != SelectedState::kZoomedIn &&
      (host_->tabs().size() != 1 ||
       selected_state_.state() != SelectedState::kAdd) &&
      private_mode_placeholder_->IsVisible(placeholder_position);

  private_mode_placeholder_->SetTargetLevels(
      placeholder_visible ? private_mode_placeholder_->mipmap()->levels() : 2);

  host_->GetUploadQueue()->Enqueue(private_mode_placeholder_);

  // TODO(peterp): Could potentially add draw order optimizations here as well.

  // Combine the lists into one and draw them.
  // Note that we don't use depth buffering so the order is important, the
  // entries from the overview must be before the big ones.
  visible_tabs.insert(visible_tabs.end(), visible_big.begin(),
                      visible_big.end());

  DrawTabs(visible_tabs);

  // Draw placeholder only if none of the currently added tabs is private.
  if (placeholder_visible)
    private_mode_placeholder_->Draw(placeholder_position, dimmer, host_);
}

bool SelectorView::OnDown(const ui::MotionEvent& e) {
  drag_direction_ = kUnknown;
  drag_active_ = true;
  gfx::Vector2dF input = host_->ToRelativeCoord(e);

  // Remember centered tab on drag start, needed to determine fling
  // behavior.
  centered_tab_on_drag_start_ =
      GetTabIndexClosestToScroll(scroll_position_, host_->tabs().size());

  switch (selected_state_.state()) {
    case SelectedState::kIdle: {
      // Anchor this gesture in case we need it later.
      scroll_acceleration_cursor_ = input;

      // Disable any running snap animation.
      snap_animate_.Reset(scroll_position_);

      // Check if it hit a tab.
      gfx::Vector2dF local_input =
          input::ConvertToLocal(input, host_->projection_scale());

      for (const TabPointer& tab : host_->tabs()) {
        if (tab->CheckHit(GetCurrentTabOffsetInSelector(tab), local_input)) {
          EnterSelectedState(tab);
          break;
        }
      }
      return true;
      break;
    }
    default:
      // NOTE: If this method returns false all touch events up to and
      // including the next 'up' will be ignored.
      return false;
  }
}

void SelectorView::OnUp(const ui::MotionEvent& e) {
  drag_direction_ = kUnknown;
  drag_active_ = false;
}

bool SelectorView::OnDragDone(const ui::MotionEvent& e) {
  switch (selected_state_.state()) {
    case SelectedState::kIdle:
    case SelectedState::kIdleNonInteractive:
      break;
    case SelectedState::kSelected: {
      // Check if it was an operation or if we should just cancel the selection.

      // Was it a "delete tab" operation?
      if (!selected_state_.tab()->is_dashboard() &&
          active_tab_y_ > kDropDeleteThreshold) {
        highlight_y_interpolator_.Reset(active_tab_y_);
        active_tab_y_ = 0.f;
        EnterDeleteState(selected_state_.tab());
        return true;
      }

      // Was it an "open tab" operation?
      if (active_tab_y_ < -kOpenThreshold) {
        EnterZoomInState();
        return true;
      }

      // No, cancel the current operation and animate the tab back to idle
      // state.
      EnterSelectedFadeOutState();
      break;
    }
    case SelectedState::kSelectedFadeOut:
    case SelectedState::kAdd:
    case SelectedState::kDelete:
    case SelectedState::kSnapToZoomIn:
    case SelectedState::kZoomIn:
    case SelectedState::kZoomedIn:
    case SelectedState::kZoomOut:
      break;
  }

  return false;
}

bool SelectorView::OnSingleTapUp(const ui::MotionEvent& e, int tap_count) {
  if (tap_count != 1) {
    return false;
  }

  gfx::Vector2dF input = host_->ToRelativeCoord(e);

  switch (selected_state_.state()) {
    case SelectedState::kIdle: {
      // Check if it hit the overview.
      int overview_hit = overview_.CheckHit(input, host_);
      if (overview_hit != -1) {
        // Do a fast scroll to get there.
        float snap_position = GetFixedTabOffsetInSelector(overview_hit).x();
        snap_animate_.SetupToPosition(snap_position, scroll_position_,
                                      host_->current_time());

        // Check if we need to update normal/private mode.
        bool hit_private_mode =
            overview_hit >= static_cast<int>(host_->public_tab_count());
        if (hit_private_mode != host_->private_mode())
          host_->UpdatePrivateMode(hit_private_mode);
        return true;
      }
      break;
    }
    case SelectedState::kIdleNonInteractive:
      break;
    case SelectedState::kSelected: {
      gfx::Vector2dF local_input =
          input::ConvertToLocal(input, host_->projection_scale());

      for (const TabPointer& tab : host_->tabs()) {
        gfx::Vector2dF offset = GetCurrentTabOffsetInSelector(tab);

        // Check if it hit a close button.
        if (!tab->is_dashboard() && tab->CheckHitClose(offset, local_input)) {
          EnterDeleteState(tab);
          return true;
        }

        // Check if it hit a tab.
        if (tab->CheckHit(offset, local_input)) {
          // Switch tab and leave this state.
          EnterZoomInState();
          return true;
        }
      }
      break;
    }
    case SelectedState::kSelectedFadeOut:
    case SelectedState::kAdd:
    case SelectedState::kDelete:
    case SelectedState::kSnapToZoomIn:
    case SelectedState::kZoomIn:
    case SelectedState::kZoomedIn:
    case SelectedState::kZoomOut:
      break;
  }

  return false;
}

bool SelectorView::OnScroll(const ui::MotionEvent& e1,
                            const ui::MotionEvent& e2,
                            const ui::MotionEvent& secondary_pointer_down,
                            float distance_x,
                            float distance_y) {
  // Restrict the drag motion to one axis. GestureDetector will
  // handles slop, so we know this is really a scroll event.
  if (drag_direction_ == kUnknown) {
    drag_direction_ =
        std::abs(distance_x) >= std::abs(distance_y) ? kHorizontal : kVertical;
  }

  float dx, dy;
  if (drag_direction_ == kHorizontal) {
    dx = -distance_x;
    dy = 0.f;
  } else {
    dx = 0.f;
    dy = -distance_y;
  }
  gfx::Vector2dF input_delta(dx / host_->screen_width(),
                             dy / host_->screen_height());

  // Adjust for coordinate space being -1..1 instead of 0..1
  input_delta.Scale(2);

  switch (selected_state_.state()) {
    case SelectedState::kIdle:
    case SelectedState::kSelected: {
      // Scale the input if we're in landscape mode.
      input_delta.Scale(1.0f / host_->projection_scale().x(),
                        1.0f / host_->projection_scale().y());

      // Only the selected tab is allowed to be dragged vertically. It's used to
      // either delete or zoom in to the current tab.
      if (selected_state_.tab())
        active_tab_y_ -= input_delta.y();

      // Panning when the dragging over the mini thumbs should match that speed.
      // Keep track of the 'cursor' since we only get deltas.
      scroll_acceleration_cursor_ += input_delta;

      // Smooth transition for the acceleration. kHighThreshold means that the
      // 'cursor' is over the mini thumbs.
      const float kLowThreshold = 0.7f;
      const float kHighThreshold = 0.8f;

      // The mini thumbs are one tenth in size so we need to match the panning
      // speed to match.
      const float kLowFactor = 1.0f;
      const float kHighFactor = 10.0f;

      float acceleration = 1.0f;
      float y = scroll_acceleration_cursor_.y();
      if (y > kHighThreshold) {
        acceleration = kHighFactor;
      } else if (y > kLowThreshold) {
        float alpha = SmoothStep(kLowThreshold, kHighThreshold, y);
        acceleration = kLowFactor + (kHighFactor - kLowFactor) * alpha;
      }

      // Handle overscroll by dampening the delta if we're outside the guard
      // band around the first and last tab.
      float left_border, right_border;
      GetScrollBorders(&left_border, &right_border);

      float over_scroll = 0.0f;
      if (scroll_position_ < right_border) {
        over_scroll = -(scroll_position_ - right_border);
      } else if (scroll_position_ > left_border) {
        over_scroll = scroll_position_ - left_border;
      }
      if (over_scroll > 0.0f) {
        // Linear increase of the dampening the further away we get.
        const float kMaxOverscroll = 0.5f;
        const float kMinScaleFactor = 1.0f;
        const float kMaxScaleFactor = 0.1f;

        // This overrides any previous acceleration.
        if (over_scroll > kMaxOverscroll) {
          acceleration = kMaxScaleFactor;
        } else {
          acceleration = (1.0f - (over_scroll / kMaxOverscroll)) *
                         (kMinScaleFactor - kMaxScaleFactor);
        }

        // Overscroll shouldn't be allowed at all when dragging over the mini
        // thumbs, but only restrict if it's moving further away over the
        // border.
        if (y > kHighThreshold) {
          if (((scroll_position_ < right_border) && (input_delta.x() <= 0)) ||
              ((scroll_position_ > left_border) && (input_delta.x() >= 0))) {
            acceleration = 0.0f;
          }
        }
      }

      scroll_position_ += input_delta.x() * acceleration;
      break;
    }
    case SelectedState::kIdleNonInteractive:
    case SelectedState::kSelectedFadeOut:
    case SelectedState::kAdd:
    case SelectedState::kDelete:
    case SelectedState::kSnapToZoomIn:
    case SelectedState::kZoomIn:
    case SelectedState::kZoomedIn:
    case SelectedState::kZoomOut:
      break;
  }

  return true;
}

bool SelectorView::OnFling(const ui::MotionEvent& e1,
                           const ui::MotionEvent& e2,
                           float velocity_x,
                           float velocity_y) {
  // These are raw pixel values, need to apply sign of projection
  // scale for RTL to work.
  velocity_x *= std::copysign(1.f, host_->projection_scale().x());
  velocity_y *= std::copysign(1.f, host_->projection_scale().y());

  if (drag_direction_ == kVertical) {
    if (selected_state_.state() != SelectedState::kSelected) {
      return false;
    }
    DCHECK(selected_state_.tab().get());
    if (velocity_y > 0.f) {
      return false;
    }
    if (selected_state_.tab()->is_dashboard()) {
      // Dashboard is not closable.
      return false;
    }
    flinger_.Start(host_->current_time(), velocity_y);
    float end_distance_px = flinger_.GetEndDistance();
    flinger_.Cancel();  // Flinger is only used to get end point.
    float end_distance = -end_distance_px / host_->screen_height();

    if (active_tab_y_ + end_distance <= kFlingDeleteThreshold) {
      return false;
    }

    EnterDeleteState(selected_state_.tab());

    // Make tab continue with current velocity.
    float end_pos = active_tab_y_ +
        static_cast<float>(kDeleteTime.InSecondsF()) * -velocity_y /
            host_->screen_height();
    highlight_y_interpolator_.Init(active_tab_y_, end_pos,
                                   host_->current_time(), kDeleteTime);
    active_tab_y_ = 0.f;

    return true;
  }

  if (!CanStartHorizontalFlingAnimation(velocity_x)) {
    return false;
  }

  // Fling happens in faded-out state.
  EnterSelectedFadeOutState();

  // Figure out where the current fling animation will end.
  flinger_.Start(host_->current_time(), velocity_x);
  float end_distance_px = flinger_.GetEndDistance();
  float end_pos = scroll_position_ + end_distance_px / host_->screen_width();

  // Match that position with the known snap positions.
  int closest_index = QuantizePosition(end_pos);
  float closest_position = -GetFixedTabOffsetInSelector(closest_index).x();
  // Adjust the velocity to make the animation end at one of the snap points.
  float delta = end_pos - closest_position;
  flinger_.Adjust(delta * host_->screen_width());

  return true;
}

void SelectorView::OnTabAdded(const TabPointer& tab) {
  // TODO(ollel): A tab can be added at any time by pop-ups or intents.
  // Only allow adding a new tab if there's no active interaction or animation
  // currently running.
  // TODO(ollel): snap animate to insertion point position if necessary.
  EnterAddState(tab);
}

void SelectorView::OnTabRemoved(const TabPointer& tab) {
  // The camera / scroll position is relative to the number of tabs.
  // If a tab before the current focus is removed then we need to adjust the
  // scroll position to still focus on the same tab.
  if (host_->GetIndexOf(tab) < GetFocusedIndex())
    scroll_position_ += kTabDistance;

  if (selected_state_.tab() == tab)
    EnterIdleState();
}

void SelectorView::OnHide() {
  // Skip animation if active tab is zoomed in already.
  if (selected_state_.state() == SelectedState::kZoomIn &&
      selected_state_.tab() == host_->active_tab())
    return;
  if (selected_state_.state() != SelectedState::kZoomedIn)
    EnterSnapToZoomInState();
}

void SelectorView::OnSetPlaceholderGraphics(jobject bitmap) {
  private_mode_placeholder_->OnCreate(bitmap, host_);
}

// -- SelectedState -----------------------------------------------------------

SelectorView::SelectedState::SelectedState() {
  Reset();
}

void SelectorView::SelectedState::Reset() {
  state_ = kIdle;
  tab_.reset();
}

void SelectorView::SelectedState::Update(State new_state) {
  state_ = new_state;
}

void SelectorView::SelectedState::Update(State new_state,
                                         const TabPointer& new_tab) {
  state_ = new_state;
  tab_ = new_tab;
}

// -- State management --------------------------------------------------------

void SelectorView::EnterIdleState() {
  selected_state_.Update(SelectedState::kIdle, TabPointer());
}

void SelectorView::UpdateIdleState() {
  UpdateScrollPositionAnimations();
}

void SelectorView::EnterSelectedState(const TabPointer& tab) {
  selected_state_.Update(SelectedState::kSelected, tab);

  // Kick off the highlight animation.
  highlight_scale_interpolator_.Init(kUnselectedScale, kSelectedScale,
                                     host_->current_time(), kHighlightDuration);
}

void SelectorView::UpdateSelectedState() {
  UpdateScrollPositionAnimations();

  highlight_scale_interpolator_.Update(host_->current_time());
}

void SelectorView::EnterSelectedFadeOutState() {
  selected_state_.Update(SelectedState::kSelectedFadeOut);

  highlight_scale_interpolator_.Init(kSelectedScale, kUnselectedScale,
                                     host_->current_time(), kHighlightDuration);
  highlight_y_interpolator_.Init(active_tab_y_, 0, host_->current_time(),
                                 kHighlightDuration);
}

void SelectorView::UpdateSelectedFadeOutState() {
  UpdateScrollPositionAnimations();

  highlight_scale_interpolator_.Update(host_->current_time());

  bool done = highlight_y_interpolator_.Update(host_->current_time());
  if (done) {
    selected_state_.Reset();
    active_tab_y_ = 0.0f;
  }
}

void SelectorView::EnterAddState(const TabPointer& tab) {
  selected_state_.Update(SelectedState::kAdd, tab);

  // The new tab has already been added to the list, teleport the camera
  // to the right to make it focus on the new tab directly (unless this is the
  // first tab, or showing the fake private tab).
  bool fake_tab_position = tab->is_private() && host_->private_tab_count() == 1;
  if ((host_->GetIndexOf(tab) > 0) && !fake_tab_position)
    scroll_position_ -= kTabDistance;
  if (host_->GetIndexOf(tab) == 0)
      scroll_position_ = 0;

  // Create an animation where all the tabs before this new one are animated
  // left one step. Then start an animation for the new tab where it animates
  // in from below. The side animation needs to be much quicker to leave enough
  // space to avoid overlap halfways through.
  const base::TimeDelta kAddTime = base::TimeDelta::FromMilliseconds(300);
  const float kFarBelow = -2.0f;
  if (fake_tab_position)
    add_offset_interpolator_.Reset(0.0f);
  else
    add_offset_interpolator_.Init(kTabDistance, 0.0f, host_->current_time(),
                                  kAddTime * 0.5f);
  add_y_interpolator_.Init(kFarBelow, 0.0f, host_->current_time(), kAddTime);

  // Reset the timer.
  add_state_timer_start_ = host_->current_time();
}

void SelectorView::UpdateAddState() {
  add_offset_interpolator_.Update(host_->current_time());
  if (add_y_interpolator_.Update(host_->current_time())) {
    // We're done with the appearance animation, but we're waiting a bit before
    // entering the zoom in state.
    const base::TimeDelta kAddWaitTime = base::TimeDelta::FromMilliseconds(400);
    if ((host_->current_time() - add_state_timer_start_) > kAddWaitTime)
      EnterZoomInState();
  }
}

void SelectorView::EnterDeleteState(const TabPointer& tab) {
  // tab_index can be different from the current index if the close icon is
  // used.
  selected_state_.Update(SelectedState::kDelete);
  delete_tab_ = tab;

  // We need to interpolate the position of each tab after this to fill the
  // space.
  int focused_index = GetFocusedIndex();
  // If delete_before_ is true tabs will be shifted from left to fill the gap,
  // which should be the case if (a) the deleted tab was positioned on left side
  // of the screen and was not the leftmost tab or (b) the deleted tab is the
  // rightmost nonprivate tab.
  // TODO(ollel): Refactor the code to not use a member variable for this.
  int tab_ix = host_->GetIndexOf(tab);
  delete_before_ =
      ((focused_index >= 0) && (tab_ix < focused_index)) ||
      (!tab->is_private() &&
        tab_ix == static_cast<int>(host_->public_tab_count()) - 1);
  delete_offset_interpolator_.Init(0.0f, 1.0f + kSpaceBetweenTabs,
                                   host_->current_time(), kDeleteTime);

  delete_opacity_interpolator_.Init(1.0f, 0.0f, host_->current_time(),
                                    kDeleteTime);
}

void SelectorView::UpdateDeleteState() {
  highlight_scale_interpolator_.Update(host_->current_time());

  // If we have an active delete tab operation then update the animation.
  delete_offset_interpolator_.Update(host_->current_time());
  highlight_y_interpolator_.Update(host_->current_time());
  if (delete_opacity_interpolator_.Update(host_->current_time())) {
    highlight_y_interpolator_.Reset(0.f);

    // Done with the animation, do the actual deleting.
    host_->RemoveTab(delete_tab_);

    if (delete_before_)
      scroll_position_ += 1.0f + kSpaceBetweenTabs;

    delete_before_ = false;
    selected_state_.Reset();

    delete_tab_.reset();
  }
}

void SelectorView::EnterSnapToZoomInState() {
  selected_state_.Update(SelectedState::kSnapToZoomIn, TabPointer());
  flinger_.Cancel();
  float snap_position = GetFixedTabOffsetInSelector(host_->active_tab()).x();
  float distance = std::abs(scroll_position_ + snap_position);
  float duration = 3.5 * (1 - exp(-0.2 * distance));
  snap_animate_.SetupToPosition(snap_position, scroll_position_,
                                host_->current_time(),
                                duration * SnapAnimate::DefaultDuration());
}

void SelectorView::UpdateSnapToZoomInState() {
  if (snap_animate_.active()) {
    UpdateScrollPositionAnimations();
  } else {
    selected_state_.Update(SelectedState::kSnapToZoomIn, host_->active_tab());
    EnterZoomInState();
  }
}

void SelectorView::EnterZoomInState() {
  // The tab ui is going to shut down and we will zoom in on the new active tab.
  TabPointer tab = selected_state_.tab();

  // Zoom in on active tab if no tab is selected.
  if (!tab)
    tab = host_->active_tab();

  // Pick first available tab if active tab has been removed.
  if (!tab) {
    CHECK(!host_->tabs().empty());
    tab = host_->tabs()[0];
  }

  SelectTab(tab);
}

void SelectorView::SelectTab(const TabPointer& tab) {
  DCHECK(tab);

  host_->SetActiveTab(tab);

  selected_state_.Update(SelectedState::kZoomIn, tab);

  // There are two ways we can enter this state, either by gesture (dragging a
  // tab down) to zoom in and then releasing, or by "tapping" the tab.

  // See if it's a manual zoom in.
  float fade = fabs(active_tab_y_);
  fade -= kThresholdY;
  fade = std::min(fade, 1.0f);
  if (fade <= -kThresholdY) {
    // Zoom from tap, not gesture.
    // Cancel out the threshold.
    fade = 0;
  }

  // Trigger an animation to zoom in fully. This is the start transform.
  gfx::Vector2dF offset = GetCurrentTabOffsetInSelector(tab);
  float s = highlight_scale_interpolator_.current_value();
  gfx::Vector2dF scale = gfx::Vector2dF(s, s * host_->viewport_height_scale());

  // This is the end transform.
  gfx::Vector2dF center_offset;
  gfx::Vector2dF fullscreen_scale;

  // Extract the end transform and manually animate the start transform if
  // necessary.
  ZoomIn(fade, &offset, &scale, &center_offset, &fullscreen_scale);

  gfx::Vector2dF crop_start, crop_end;
  Cropping(fade, &crop_start, &crop_end);

  // Set up the animation.
  base::TimeDelta duration = kZoomDuration * (1.0f - fade);
  zoom_offset_interpolator_.Init(offset, center_offset, host_->current_time(),
                                 duration);
  zoom_scale_interpolator_.Init(scale, fullscreen_scale, host_->current_time(),
                                duration);
  zoom_crop_interpolator_.Init(crop_start, crop_end, host_->current_time(),
                               duration);
  zoom_opacity_interpolator_.Init(1.0f - fade, kMinChromeOpacity,
                                  host_->current_time(), duration);
}

void SelectorView::UpdateZoomInState() {
  highlight_scale_interpolator_.Update(host_->current_time());

  // If we have an active open tab operation then update the animation.
  bool done = zoom_offset_interpolator_.Update(host_->current_time());
  zoom_scale_interpolator_.Update(host_->current_time());
  zoom_opacity_interpolator_.Update(host_->current_time());
  zoom_crop_interpolator_.Update(host_->current_time());
  if (done) {
    EnterZoomedInState();
  }
}

void SelectorView::EnterZoomedInState() {
  selected_state_.Update(SelectedState::kZoomedIn);
  SwitchToPageView();
}

void SelectorView::UpdateZoomedInState() {
  // Stay zoomed in until view becomes invisible.
}

void SelectorView::EnterZoomOutState() {
  // Figure out the right tab index.
  int closest_index = -1;
  CalcSnapDelta(&closest_index);
  selected_state_.Update(SelectedState::kZoomOut,
                         host_->tabs().at(closest_index));

  // Trigger an animation to zoom out fully. This is the end transform.
  gfx::Vector2dF offset = GetCurrentTabOffsetInSelector(selected_state_.tab());
  float scale = highlight_scale_interpolator_.current_value();
  gfx::Vector2dF scale_2d =
      gfx::Vector2dF(scale, scale * host_->viewport_height_scale());

  // Get the transform for a fullscreen quad depending on portrait/landscape
  // mode. This is the start transform.
  gfx::Vector2dF center_offset;
  gfx::Vector2dF fullscreen_scale;
  GetFullscreenTransform(&center_offset, &fullscreen_scale);

  // We have both end points to set up the transform animation.
  zoom_offset_interpolator_.Init(center_offset, offset, host_->current_time(),
                                 kZoomDuration);
  zoom_scale_interpolator_.Init(fullscreen_scale, scale_2d,
                                host_->current_time(), kZoomDuration);
  zoom_opacity_interpolator_.Init(kMinChromeOpacity, 1.0f,
                                  host_->current_time(), kZoomDuration);

  gfx::Vector2dF crop_start, crop_end;
  Cropping(1.0f, &crop_end, &crop_start);

  zoom_crop_interpolator_.Init(crop_start, crop_end, host_->current_time(),
                               kZoomDuration);
}

void SelectorView::UpdateZoomOutState() {
  highlight_scale_interpolator_.Update(host_->current_time());

  // If we have an active open tab operation then update the animation.
  bool done = zoom_offset_interpolator_.Update(host_->current_time());
  zoom_scale_interpolator_.Update(host_->current_time());
  zoom_opacity_interpolator_.Update(host_->current_time());
  zoom_crop_interpolator_.Update(host_->current_time());
  if (done) {
    // Switch back to idle state.
    selected_state_.Reset();
    return;
  }
}

void SelectorView::LimitScrollBorders(float* left_border,
                                      float* right_border) const {
  // Only applicable when flinging.
  DCHECK(!drag_active_);
  DCHECK(flinger_.is_flinging());

  if (!host_->public_tab_count() || !host_->private_tab_count()) {
    return;
  }

  int last_normal_tab_idx = host_->public_tab_count() - 1;
  int first_private_tab_idx = last_normal_tab_idx + 1;
  bool was_private = centered_tab_on_drag_start_ > last_normal_tab_idx;
  bool allow_crossing_private_boundary =
      centered_tab_on_drag_start_ == last_normal_tab_idx ||
      centered_tab_on_drag_start_ == first_private_tab_idx;

  if (was_private) {
    *left_border = -GetFixedTabOffsetInSelector(allow_crossing_private_boundary
                                                    ? last_normal_tab_idx
                                                    : first_private_tab_idx)
                        .x();
  } else {
    *right_border = -GetFixedTabOffsetInSelector(allow_crossing_private_boundary
                                                     ? first_private_tab_idx
                                                     : last_normal_tab_idx)
                         .x();
  }
}

void SelectorView::UpdateScrollPositionAnimations() {
  // Check which tab we're focusing on right now.
  int snap_target = -1;
  float snap_delta = CalcSnapDelta(&snap_target);

  // Update scroll position from fling or snapping.
  if (!drag_active_) {
    if (flinger_.is_flinging()) {
      // Update position from fling.

      // Abruptly cancel any fling animation that goes outside the borders and
      // let snap animation take over.
      float left_border, right_border;
      GetScrollBorders(&left_border, &right_border);
      LimitScrollBorders(&left_border, &right_border);
      if ((scroll_position_ < right_border) ||
          (scroll_position_ > left_border)) {
        flinger_.Cancel();
      } else {
        float delta_px = flinger_.GetDelta(host_->current_time());
        float delta = delta_px / host_->screen_width();
        scroll_position_ += delta;
      }
    } else if (snap_animate_.active()) {
      // Update position from snapping.
      scroll_position_ = snap_animate_.Animate(host_->current_time());
    } else {
      // Check if we're centered over a tab and if not animate to that position.
      snap_animate_.Setup(snap_target, snap_delta, scroll_position_,
                          host_->current_time());
    }
  }

  // Update the UI if we manually pan between normal and private mode tabs.
  if (!snap_animate_.active()) {
    // Detect if we're currently focusing the private mode section.
    bool hit_private_mode = false;
    if (snap_target >= static_cast<int>(host_->tabs().size()))
      hit_private_mode = true;
    else if ((host_->private_tab_count()) &&
             (snap_target >= static_cast<int>(host_->public_tab_count())))
      hit_private_mode = true;

    // Only update if it's a change.
    if (hit_private_mode != host_->private_mode())
      host_->UpdatePrivateMode(hit_private_mode);
  }
}

void SelectorView::ZoomIn(float fade,
                          gfx::Vector2dF* offset,
                          gfx::Vector2dF* scale,
                          gfx::Vector2dF* center_offset,
                          gfx::Vector2dF* fullscreen_scale) {
  // This is the end transform.
  GetFullscreenTransform(center_offset, fullscreen_scale);

  if (fade > 0) {
    // This animation start from somewhere in the middle. Manually jump forward
    // in the animation to match the current transform.

    gfx::Vector2dF offset_delta = *center_offset - *offset;
    offset_delta.Scale(fade);
    *offset += offset_delta;

    scale->set_x(scale->x() + (fullscreen_scale->x() - scale->x()) * fade);
    scale->set_y(scale->y() + (fullscreen_scale->y() - scale->y()) * fade);
  }
}

void SelectorView::Cropping(float fade,
                            gfx::Vector2dF* start,
                            gfx::Vector2dF* end) {
  const TabSelectorRenderer::TabPointer tab = selected_state_.tab();
  const std::shared_ptr<GLMipMap> tab_mipmap = tab->mipmap();

  bool tab_landscape =
      tab_mipmap->original_width() > tab_mipmap->original_height();

  *start = tab->Cropping();

  if (host_->portrait_mode() && tab_landscape) {
    *end = *start;
  } else {
    *end = gfx::Vector2dF(1.0f, 1.0f);
    gfx::Vector2dF cropping_delta = gfx::Vector2dF(1.0f, 1.0f) - *end;
    cropping_delta.Scale(fade);
    *end += cropping_delta;
  }
}

// -- Rendering ---------------------------------------------------------------

VisibleTabsContainer SelectorView::CullTabs(float dimmer) {
  VisibleTabsContainer result;

  // Cull the background tabs.

  float height_scale = host_->viewport_height_scale();

  for (const TabPointer& tab : host_->tabs()) {
    // We need to draw the highlighted tab after the background tabs since there
    // might be overlap otherwise and we don't enable the depth buffer.
    if (tab == selected_state_.tab())
      continue;

    // Fade out the delete tab if it was closed with the button but not
    // highlighted.
    float opacity = 1.0f;

    if (delete_tab_ == tab)
      opacity = delete_opacity_interpolator_.current_value();

    gfx::Vector4dF color(kSelectedIntensity, kSelectedIntensity,
                         kSelectedIntensity, opacity);

    gfx::Vector2dF offset = GetCurrentTabOffsetInSelector(tab);
    gfx::Vector2dF scale(kUnselectedScale, kUnselectedScale * height_scale);

    float d = 0.0f;
    if (!host_->private_mode() && tab->is_private())
      d = dimmer;

    if (tab->IsVisible(offset, scale))
      result.push_back({tab, offset, scale, color, tab->Cropping(), 1.0f, d});
  }

  // Cull the foreground tab in highlight mode.
  float chrome_opacity = 1.0f;
  const TabPointer& tab = selected_state_.tab();
  if (tab) {
    // The tab can be transformed in various ways depending on the current
    // state.
    gfx::Vector2dF offset(0, 0);
    gfx::Vector2dF scale(1, height_scale);
    float opacity = 1.0f;
    if (delete_tab_ == tab) {
      opacity = delete_opacity_interpolator_.current_value();
      chrome_opacity = opacity;
    }
    gfx::Vector2dF cropping = tab->Cropping();

    switch (selected_state_.state()) {
      case SelectedState::kIdle:
      case SelectedState::kIdleNonInteractive:
      case SelectedState::kSnapToZoomIn:
        // We should not get here at all.
        LOG(FATAL) << "Internal error: selected tab in idle state.";
        break;
      case SelectedState::kAdd:
        // Animate in the tab from below.
        offset = GetCurrentTabOffsetInSelector(tab);
        offset.set_y(offset.y() + add_y_interpolator_.current_value());
        break;
      case SelectedState::kSelected:
      case SelectedState::kSelectedFadeOut:
      case SelectedState::kDelete: {
        offset = GetCurrentTabOffsetInSelector(tab);
        float s = highlight_scale_interpolator_.current_value();
        scale = gfx::Vector2dF(s, s * height_scale);

        // Use the y offset to fade out as well, but with a threshold.
        // This can be overridden when fading out the selection.
        float y = active_tab_y_;
        if (selected_state_.state() == SelectedState::kSelectedFadeOut ||
            selected_state_.state() == SelectedState::kDelete) {
          y = highlight_y_interpolator_.current_value();
        }

        float fade = fabs(y);
        if (fade > kThresholdY) {
          fade -= kThresholdY;
          fade = cc::MathUtil::ClampToRange(fade, 0.0f, 1.0f);

          if (y < 0) {
            // Zooming into the tab.
            gfx::Vector2dF center_offset;
            gfx::Vector2dF fullscreen_scale;
            ZoomIn(fade, &offset, &scale, &center_offset, &fullscreen_scale);

            if (!host_->portrait_mode()) {
              gfx::Vector2dF cropping_delta =
                  gfx::Vector2dF(1.0f, 1.0f) - cropping;
              cropping_delta.Scale(fade);
              cropping += cropping_delta;
            }

            fade = 1.0f - fade;
            chrome_opacity = std::max(fade, kMinChromeOpacity);
          } else {
            // Deleting the tab.
            offset.set_y(offset.y() + y - kThresholdY);
            fade = 1.0f - fade;

            // Also fade out the delete tab.
            if (delete_tab_ == selected_state_.tab())
              fade *= delete_opacity_interpolator_.current_value();

            opacity = fade;
            chrome_opacity = opacity;
          }
        }
        break;
      }
      case SelectedState::kZoomIn:
      case SelectedState::kZoomedIn:
      case SelectedState::kZoomOut:
        // This is used for automatic zoom in after tapping a tab.
        offset = zoom_offset_interpolator_.current_value();
        scale = zoom_scale_interpolator_.current_value();
        cropping = zoom_crop_interpolator_.current_value();
        chrome_opacity = zoom_opacity_interpolator_.current_value();
        break;
    }

    gfx::Vector4dF color(1, 1, 1, opacity);

    if (tab->IsVisible(offset, scale))
      result.push_back(
          {tab, offset, scale, color, cropping, chrome_opacity, 0.0f});
  }

  return result;
}

void SelectorView::DrawTabs(const VisibleTabsContainer& tabs) {
  for (auto t : tabs)
    t.tab->Draw(t.offset, t.scale, t.color, t.cropping, t.chrome_opacity,
                t.dimmed);
}

void SelectorView::GetFullscreenTransform(gfx::Vector2dF* center_offset,
                                          gfx::Vector2dF* fullscreen_scale) {
  const float kFullscreenScale = 2.0f;

  // We want to fill the part of the viewport between the top (i.e. omnibar) and
  // bottom (i.e. menu button row) sections.
  float height_scale = host_->viewport_height_scale();
  // The offset depends on the relative height of the sections.
  float height_offset =
      (host_->viewport_bottom_height() - host_->viewport_top_height()) /
      static_cast<float>(host_->screen_height());

  if (host_->portrait_mode()) {
    *center_offset =
        gfx::Vector2dF(0.0f, host_->GetTabChromeScale().y() + height_offset);
    *fullscreen_scale =
        gfx::Vector2dF(kFullscreenScale, kFullscreenScale * height_scale);
  } else {
    float inv_projection_scale = host_->is_rtl()
                                     ? -1.0f / host_->projection_scale().x()
                                     : 1.0f / host_->projection_scale().x();

    // We need to come up with an approximation if the host and screenshot
    // orientation doesn't match.
    const std::shared_ptr<GLMipMap> mipmap = selected_state_.tab()->mipmap();
    bool tab_portrait = mipmap->original_width() < mipmap->original_height();
    if (tab_portrait) {
      // Invert the scale factor from the projection scale to make it fill up
      // a virtual screen that is proportial to how wide it is in landscape.
      // This means that the bottom part will extend far below the screen.
      *center_offset =
          gfx::Vector2dF(0, -(inv_projection_scale - 1) + height_offset);
      float s = kFullscreenScale * inv_projection_scale;
      fullscreen_scale->set_x(s);
      fullscreen_scale->set_y(s);
    } else {
      // This is an animation from small portrait size to normal landscape size
      // while revealing more and more of the cropped image.
      *center_offset =
          gfx::Vector2dF(0, host_->GetTabChromeScale().y() + height_offset);
      fullscreen_scale->set_x(kFullscreenScale * inv_projection_scale);
      fullscreen_scale->set_y(kFullscreenScale * height_scale);
    }
  }
}

// -- Internal ----------------------------------------------------------------

void SelectorView::Update() {
  switch (selected_state_.state()) {
    case SelectedState::kIdle:
    case SelectedState::kIdleNonInteractive:
      UpdateIdleState();
      break;
    case SelectedState::kSelected:
      UpdateSelectedState();
      break;
    case SelectedState::kSelectedFadeOut:
      UpdateSelectedFadeOutState();
      break;
    case SelectedState::kAdd:
      UpdateAddState();
      break;
    case SelectedState::kDelete:
      UpdateDeleteState();
      break;
    case SelectedState::kSnapToZoomIn:
      UpdateSnapToZoomInState();
      break;
    case SelectedState::kZoomIn:
      UpdateZoomInState();
      break;
    case SelectedState::kZoomedIn:
      UpdateZoomedInState();
      break;
    case SelectedState::kZoomOut:
      UpdateZoomOutState();
      break;
  }
}

void SelectorView::SwitchToPageView() {
  host_->SwitchToPageView();
}

gfx::Vector2dF SelectorView::GetCurrentTabOffsetInSelector(
    const TabPointer& tab) const {
  // Adjust position of all tabs if we're in the middle of a adding or deleting.
  // This means sliding the tabs before/after to the left to fill the empty
  // space.
  float delta = 0.0f;
  if ((selected_state_.state() == SelectedState::kAdd) &&
      (host_->Distance(tab, selected_state_.tab()) > 0)) {
    delta = add_offset_interpolator_.current_value();
  } else if (host_->GetIndexOf(delete_tab_) != -1) {
    if (delete_before_) {
      if (host_->Distance(tab, delete_tab_) > 0)
        delta = delete_offset_interpolator_.current_value();
    } else {
      if (host_->Distance(tab, delete_tab_) < 0)
        delta = -delete_offset_interpolator_.current_value();
    }
  }

  gfx::Vector2dF current_position(scroll_position_ + delta, 0.0f);
  return current_position + GetFixedTabOffsetInSelector(tab);
}

void SelectorView::ResetVariables() {
  selected_state_.Reset();
  delete_tab_.reset();
  scroll_position_ = 0;
  active_tab_y_ = 0;
  scroll_acceleration_cursor_ = gfx::Vector2dF(0, 0);
  snap_animate_.Reset(scroll_position_);
  highlight_scale_interpolator_.Reset(kUnselectedScale);
}

gfx::Vector2dF SelectorView::GetFixedTabOffsetInSelector(int index) const {
  // Private tabs are offsetted a bit.
  float gap = index >= static_cast<int>(host_->public_tab_count())
                  ? kSpaceBetweenNormalAndPrivateMode
                  : 0;

  const float kTabOffsetY = 0.2f;
  return gfx::Vector2dF(index * (1.0f + kSpaceBetweenTabs) + gap, kTabOffsetY);
}

gfx::Vector2dF SelectorView::GetFixedTabOffsetInSelector(
    const TabPointer& tab) const {
  // Get index of tab or leftmost position if tab has been removed.
  int index = std::max(host_->GetIndexOf(tab), 0);
  return GetFixedTabOffsetInSelector(index);
}

float SelectorView::GetSpaceBetweenTabs() {
  return kSpaceBetweenTabs;
}

float SelectorView::GetSpaceBetweenNormalAndPrivateMode() {
  return kSpaceBetweenNormalAndPrivateMode;
}

void SelectorView::GetScrollBorders(float* left_border, float* right_border) {
  const float kGuardBand = 0.1f;
  size_t num_tabs = host_->tabs().size();
  // Allow scroll to ghost position if there are no private tabs.
  if (!host_->private_tab_count())
    ++num_tabs;

  *left_border = kGuardBand;
  *right_border = -((num_tabs - 1) * (1.0f + kSpaceBetweenTabs) + kGuardBand +
                    kSpaceBetweenNormalAndPrivateMode);
}

bool SelectorView::CanStartHorizontalFlingAnimation(float velocity_x) {
  float left_border, right_border;
  GetScrollBorders(&left_border, &right_border);

  // Don't start a fling acceleration if it would take us even further away from
  // the legal range, let the snap animation take over directly instead.
  if (scroll_position_ < right_border) {
    if (velocity_x < 0.0f) {
      return false;
    }
  } else if (scroll_position_ > left_border) {
    if (velocity_x > 0.0f) {
      return false;
    }
  }

  return true;
}

void SelectorView::SetNonInteractive() {
  selected_state_.Update(SelectedState::kIdleNonInteractive);
}

int SelectorView::GetFocusedIndex() const {
  int closest_index = -1;
  CalcSnapDelta(&closest_index);
  return closest_index;
}

void SelectorView::ScrollToPrivateMode(bool enable) {
  // Figure out which index we should target.
  int target_index = host_->public_tab_count();
  if (!enable)
    --target_index;

  // Calculate the actual scroll position that means.
  float snap_position = GetFixedTabOffsetInSelector(target_index).x();
  if (snap_position != scroll_position_) {
    // Cancel any potential fling animation that might be active and start a
    // new snap animation instead.
    flinger_.Cancel();
    snap_animate_.SetupToPosition(snap_position, scroll_position_,
                                  host_->current_time());
  }
}

float SelectorView::CalcSnapDelta(int* index) const {
  int closest_index = -1;
  float closest_delta = 0;

  // Note that the scroll_position is essentially acting like a camera here.
  // That means that we have to take the transformed position of each tab and
  // compare it to view space.

  // Find the closest snap position.
  closest_index = -1;
  closest_delta = FLT_MAX;
  float closest_distance = FLT_MAX;

  // If we're in private mode and we don't yet have any private tabs then a
  // fake one is shown.
  size_t actual_num_tabs = host_->tabs().size();
  if (!host_->private_tab_count())
    ++actual_num_tabs;

  // Normal tabs are always shown.
  for (size_t i = 0; i < actual_num_tabs; ++i) {
    float snap_position = GetFixedTabOffsetInSelector(i).x();

    float delta = snap_position + scroll_position_;
    float distance = fabs(delta);
    if (distance < closest_distance) {
      closest_index = i;
      closest_delta = delta;
      closest_distance = distance;
    }
  }

  if (index)
    *index = closest_index;

  return closest_delta;
}

int SelectorView::QuantizePosition(float scroll_position) const {
  // Note that the scroll_position is essentially acting like a camera here.
  // That means that we have to take the transformed position of each tab and
  // compare it to view space.

  // If we're in private mode and we don't yet have any private tabs then a
  // fake one is shown.
  size_t actual_num_tabs = host_->tabs().size();
  // Allow scroll to ghost position if there are no private tabs.
  if (!host_->private_tab_count())
    ++actual_num_tabs;

  // Find the closest snap position.
  int target_index =
      GetTabIndexClosestToScroll(scroll_position, actual_num_tabs);
  return target_index;
}

int SelectorView::GetTabIndexClosestToScroll(float scroll_x,
                                             unsigned num_tabs) const {
  int closest_index = -1;
  float closest_distance = FLT_MAX;
  for (size_t i = 0; i < num_tabs; ++i) {
    float snap_position = GetFixedTabOffsetInSelector(i).x();
    float delta = snap_position + scroll_x;
    float distance = fabs(delta);
    if (distance < closest_distance) {
      closest_index = i;
      closest_distance = distance;
    }
  }
  return closest_index;
}

}  // namespace tabui
}  // namespace opera
