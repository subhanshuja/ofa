// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_UI_TABS_SELECTOR_VIEW_H_
#define CHILL_BROWSER_UI_TABS_SELECTOR_VIEW_H_

#include <jni.h>

#include <memory>

#include "ui/events/gesture_detection/gesture_listeners.h"

#include "chill/browser/ui/tabs/background_theme.h"
#include "chill/browser/ui/tabs/flinger.h"
#include "chill/browser/ui/tabs/overview.h"
#include "chill/browser/ui/tabs/snap_animate.h"
#include "chill/browser/ui/tabs/tab_view.h"

namespace opera {
namespace tabui {

class PrivateModePlaceholder;
class TabSelectorRenderer;
class TabView;

// Show all the tabs and let the user delete or add new tabs and select which
// one to activate for fullscreen view.
class SelectorView : public ui::SimpleGestureListener {
 public:
  typedef std::shared_ptr<TabView> TabPointer;

  explicit SelectorView(TabSelectorRenderer* host);
  ~SelectorView();

  bool OnCreate();
  void OnGLContextLost();
  void OnSurfaceChanged(int width, int height);
  void OnEnter();
  void OnDraw();
  void OnTabAdded(const TabPointer& tab);
  void OnTabRemoved(const TabPointer& tab);
  void OnHide();

  // Change the selected tab and start the zoom in transition.
  void SelectTab(const TabPointer& tab);

  void OnSetPlaceholderGraphics(jobject bitmap);

  // Check if we should allow a horizontal fling animation from the
  // current input event.
  bool CanStartHorizontalFlingAnimation(float velocity_x);

  void FlingHorizontal(float velocity_x);

  // Block all interactions.
  void SetNonInteractive();

  // Return the index of the currently centered/focused tab.
  int GetFocusedIndex() const;

  void ScrollToPrivateMode(bool enable);

  // Find the offset for a tab disregarding all animations or the current scroll
  // position. This includes the space between tabs.
  gfx::Vector2dF GetFixedTabOffsetInSelector(int index) const;
  gfx::Vector2dF GetFixedTabOffsetInSelector(const TabPointer& tab) const;

  // Return how far away we're from the closest snap position at the moment.
  float CalcSnapDelta(int* index = NULL) const;

  // Snap the scroll position to be centered on one of the tabs.
  int QuantizePosition(float scroll_position) const;

  // Returns index of tab closest to current scroll position.
  int GetTabIndexClosestToScroll(float scroll_x, unsigned num_tabs) const;

  static float GetSpaceBetweenTabs();
  static float GetSpaceBetweenNormalAndPrivateMode();

  // GestureListener implementation.
  bool OnDown(const ui::MotionEvent& e) override;
  bool OnScroll(const ui::MotionEvent& e1,
                const ui::MotionEvent& e2,
                const ui::MotionEvent& secondary_pointer_down,
                float distance_x,
                float distance_y) override;
  bool OnSingleTapUp(const ui::MotionEvent& e, int tap_count) override;
  bool OnFling(const ui::MotionEvent& e1,
               const ui::MotionEvent& e2,
               float velocity_x,
               float velocity_y) override;

  // Additional input methods called from TabSelectorRenderer that are
  // not part of the GestureListener interface.

  // Always called on up, after any gesture is triggered.
  void OnUp(const ui::MotionEvent& e);
  // Called when up event did not trigger any gesture, after OnUp.
  bool OnDragDone(const ui::MotionEvent& e);

 private:
  // -- State management ------------------------------------------------------

  //    [Browser showing an active page]
  //                  ||
  //                  \/
  //               [ZoomOut]
  //                  ||
  //                  \/
  //   ===========> [Idle] <==========
  //   ||             ||            ||
  //   ||             \/            ||
  //   ||         [Selected]        ||
  //   ||       //    ||   \\       ||
  //   ||     //      ||     \\     ||
  //   ||   //        ||       \\   ||
  //   ||  \/         \/        \/  ||
  //  [Delete]     [ZoomIn] [SelectedFadeOut]
  //                  ||
  //                  \/
  //    [Browser showing an active page]

  // We can interact with one of the tabs. This describes in what state that
  // interaction is.
  class SelectedState {
   public:
    enum State {
      // No tab is currently selected, mainly panning.
      kIdle,
      // No interaction allowed, waiting for a roundtrip to complete.
      kIdleNonInteractive,
      // One of the tabs is touched by input is shown as highlighted by scaling
      // it up and decreasing the intensity of the other tabs.
      kSelected,
      // Fade out the selection highlight and scale before returning to idle
      // state.
      kSelectedFadeOut,
      // We're in the middle of an animation to add a tab.
      kAdd,
      // We're in the middle of an animation to remove a tab. The tab is to be
      // deleted is rendered on top of the other ones, is offsetted upwards and
      // is  fading out.
      kDelete,
      // Snap-animating to active tab that should be zoomed in.
      kSnapToZoomIn,
      // In the middle of an animation to zoom into a tab.
      kZoomIn,
      // Stay zoomed in on tab waiting for view to become invisible.
      kZoomedIn,
      // In the middle of an animation to zoom out to page view.
      kZoomOut
    };

    SelectedState();

    void Reset();
    void Update(State new_state);
    void Update(State new_state, const TabPointer& new_tab);

    State state() const { return state_; }
    const TabPointer& tab() const { return tab_; }

   private:
    State state_;
    TabPointer tab_;  // Which tab is interacted with, or null if none.
  };
  SelectedState selected_state_;

  void EnterIdleState();
  void UpdateIdleState();

  void EnterSelectedState(const TabPointer& tab);
  void UpdateSelectedState();

  void EnterSelectedFadeOutState();
  void UpdateSelectedFadeOutState();

  void EnterAddState(const TabPointer& tab);
  void UpdateAddState();

  void EnterDeleteState(const TabPointer& tab);
  void UpdateDeleteState();

  void EnterSnapToZoomInState();
  void UpdateSnapToZoomInState();

  void EnterZoomInState();
  void UpdateZoomInState();

  void EnterZoomedInState();
  void UpdateZoomedInState();

  void EnterZoomOutState();
  void UpdateZoomOutState();

  void RequestUpload(const VisibleTabsContainer& big_tabs,
                     const VisibleTabsContainer& small_tabs);

  // If set, then we're in the middle of an animation to remove a tab. The tab
  // that is to be deleted is rendered on top of the other ones, is offsetted
  // upwards and is fading out. This can differ from selected_state_.index()
  // if the close button is used instead of the input gesture.
  // TODO(peterp): Try to remove this variable, selected_state_.index() should
  // be enough.
  std::shared_ptr<TabView> delete_tab_;

  // Remember if the tab being deleted was before or after the focused tab.
  bool delete_before_;

  // There should be a short pause between animating the new tab in and zooming
  // into it. This is the timer used to keep track of it.
  base::Time add_state_timer_start_;

  // Limits scroll borders based on fling direction and centered tab
  // at touch start to conform to desired fling behavior.
  void LimitScrollBorders(float* left_border, float* right_border) const;

  // Let fling and snap animations update the scroll position.
  void UpdateScrollPositionAnimations();

  // Take the start transform, calculate the end transform and based on the
  // 'fade' parameter manually animate the start transform if necessary.
  void ZoomIn(float fade,
              gfx::Vector2dF* offset,
              gfx::Vector2dF* scale,
              gfx::Vector2dF* center_offset,
              gfx::Vector2dF* fullscreen_scale);

  void Cropping(float fade, gfx::Vector2dF* start, gfx::Vector2dF* end);

  // -- Rendering -------------------------------------------------------------

  // Like a minimap this shows miniature versions of all the active tabs to
  // facilitate quick navigation.
  Overview overview_;

  // Background pattern.
  BackgroundTheme background_theme_;

  std::shared_ptr<PrivateModePlaceholder> private_mode_placeholder_;

  // The current camera position, (0.0) is centered on the first tab and
  // (-(num_tabs - 1) * Constant) is centered on the last tab.
  float scroll_position_;

  // Index of tab closest to center on start of drag.
  int centered_tab_on_drag_start_;

  // The current vertical position of the active/highlight tab. This is a
  // temporary values used during a zoom in or delete tab gesture.
  float active_tab_y_;

  // Determine the set of visible tabs together with the draw attributes like
  // transform and highlights effects.
  VisibleTabsContainer CullTabs(float dimmer);

  // Draw the set of visible tabs.
  void DrawTabs(const VisibleTabsContainer& tabs);

  // Calculate the transform to render a tab in fullscreen.
  // This function hides the differences between portrait or landscape mode
  // where there's cropping going on.
  void GetFullscreenTransform(gfx::Vector2dF* center_offset,
                              gfx::Vector2dF* fullscreen_scale);

  // -- Input -----------------------------------------------------------------

  // If we're dragging in the area below the overview then we want to scroll
  // faster. This is the anchor point for that.
  gfx::Vector2dF scroll_acceleration_cursor_;

  // Encapsulates the ease-in-ease-out interpolation to make the scroll position
  // snap to a position that centers on the current tab.
  SnapAnimate snap_animate_;

  // Functionality for computing the duration, end point and
  // intermediate positions of a deceleration animation.
  Flinger flinger_;

  // Input.
  enum DragDirection { kUnknown, kHorizontal, kVertical };
  bool drag_active_;
  DragDirection drag_direction_;

  // -- Internal animation parameters -----------------------------------------
  // TODO(peterp): make structs for related values and reduce number of
  // interpolators?
  // TODO(peterp): combine interpolators, no need to have multiple scale or
  // offsets
  // that aren't used simultaneously.
  Interpolator<float> highlight_scale_interpolator_;
  Interpolator<float> highlight_y_interpolator_;

  Interpolator<float> delete_offset_interpolator_;
  Interpolator<float> delete_opacity_interpolator_;

  Interpolator<float> add_offset_interpolator_;
  Interpolator<float> add_y_interpolator_;

  Interpolator<gfx::Vector2dF> zoom_offset_interpolator_;
  Interpolator<gfx::Vector2dF> zoom_scale_interpolator_;
  Interpolator<gfx::Vector2dF> zoom_crop_interpolator_;
  Interpolator<float> zoom_opacity_interpolator_;

  // -- Internal --------------------------------------------------------------

  TabSelectorRenderer* host_;

  // Update all animations and internal states.
  void Update();

  // Leave this state and enter the page view instead.
  void SwitchToPageView();

  // Find the current offset for a tab after all animations have been taken into
  // account.
  gfx::Vector2dF GetCurrentTabOffsetInSelector(const TabPointer& tab) const;

  // The end positions of the scroll position.
  void GetScrollBorders(float* left_border, float* right_border);

  // Restore all non-persistent variables to default.
  void ResetVariables();
};

}  // namespace tabui
}  // namespace opera

#endif  // CHILL_BROWSER_UI_TABS_SELECTOR_VIEW_H_
