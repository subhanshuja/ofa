// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_UI_TABS_TAB_SELECTOR_RENDERER_H_
#define CHILL_BROWSER_UI_TABS_TAB_SELECTOR_RENDERER_H_

#include <jni.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "base/time/time.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "ui/events/gesture_detection/gesture_detector.h"
#include "ui/gfx/geometry/vector2d_f.h"

#include "chill/browser/ui/tabs/thumbnail_cache.h"

namespace opera {

class GLGeometry;
class GLShader;

namespace tabui {

class SelectorView;
class TabView;
class UploadQueue;

class TabSelectorRenderer {
 public:
  typedef std::shared_ptr<TabView> TabPointer;

  explicit TabSelectorRenderer(const ThumbnailCache& thumbnail_cache,
                               const ThumbnailCache& chrome_cache,
                               const float title_ratio,
                               bool is_rtl);
  virtual ~TabSelectorRenderer();

  // -- GLSurfaceView interaction --
  void OnSurfaceCreated();
  void OnSurfaceChanged(int width, int height);
  void OnDrawFrame();
  void OnSetViewportLimits(int top_height, int bottom_height);
  void OnSetTouchSlop(int touch_slop);
  void OnSetPlaceholderGraphics(jobject bitmap);
  void OnTouchEvent(int action, float x, float y);

  // -- UI interaction --
  void OnTabsSetup(int id, bool is_dashboard, bool is_private);
  void OnTabsSetup(int id, bool is_dashboard, bool is_private, int after_id);
  void OnTabAdded(int id, bool is_dashboard, bool is_private);
  void OnTabAdded(int id, bool is_dashboard, bool is_private, int after_id);
  void OnTabRemoved(int id);
  void OnActiveTabChanged(int id);
  void OnShow();
  void OnHide();
  void OnSetPrivateMode(bool enable);
  void SelectTab(int id);

  int GetFocusedTabId() const;

  // -- Internal --

  const gfx::Vector2dF GetTabChromeScale() const {
    return gfx::Vector2dF(1.0f, title_ratio_);
  }

  // Property access.
  const base::Time& current_time() const { return current_time_; }

  // If we're in portrait or landscape mode.
  bool portrait_mode() const { return portrait_mode_; }

  // Change the current view.
  void SwitchToPageView();

  // Access the tabs.
  const std::vector<TabPointer>& tabs() const { return tabs_; }

  // Get index of a tab.
  int GetIndexOf(const TabPointer& tab);

  // Distance between two tabs.
  int Distance(const TabPointer& first, const TabPointer& last);

  const TabPointer& active_tab() const { return active_tab_; }
  void SetActiveTab(const TabPointer& tab);
  void RemoveTab(const TabPointer& tab);

  // ...
  size_t public_tab_count() const { return public_tab_count_; }
  size_t private_tab_count() const { return tabs_.size() - public_tab_count_; }
  bool private_mode() const { return private_mode_; }
  void UpdatePrivateMode(bool enable);

  // Input.
  gfx::Vector2dF ToRelativeCoord(const ui::MotionEvent& e);

  // Access the queue for texture uploads.
  const std::shared_ptr<UploadQueue>& GetUploadQueue() const {
    return upload_queue_;
  }

  // -- Render resources --
  GLGeometry* image_quad() { return image_quad_.get(); }
  GLGeometry* outline_quad() { return outline_quad_.get(); }
  GLGeometry* solid_quad() { return solid_quad_.get(); }
  GLShader* image_shader() { return image_shader_.get(); }
  GLShader* color_shader() { return color_shader_.get(); }

  bool is_rtl() const { return is_rtl_; }
  bool is_low_res_device() const { return is_low_res_device_; }
  int screen_width() const { return screen_width_; }
  int screen_height() const { return screen_height_; }
  int viewport_height() const {
    return screen_height_ - viewport_top_height_ - viewport_bottom_height_;
  }
  int viewport_top_height() const { return viewport_top_height_; }
  int viewport_bottom_height() const { return viewport_bottom_height_; }
  const gfx::Vector2dF& projection_scale() const { return projection_scale_; }
  float viewport_height_scale() const;

  virtual void DoZoomedIn() = 0;
  virtual void DoRemoveTab(int id) = 0;
  virtual void DoSetActiveTab(int id) = 0;
  virtual void DoSetPrivateMode(bool privateMode) = 0;

 private:
  static bool IsLowResDevice();

  void OnTabsSetup(int index, int id, bool is_dashboard, bool is_private);

  // If thumbnails are in rtl mode.
  const bool is_rtl_;

  bool initialized_;
  bool hidden_;
  bool first_draw_;
  bool surface_created_;

  // Only poll the current time once and reuse that value everywhere to avoid
  // drift inside a rendered frame.
  base::Time start_time_;
  base::Time current_time_;

  // If we're in portrait or landscape mode.
  bool portrait_mode_;

  // Ratio of title relative to total height of tab preview
  const float title_ratio_;

  // Queue for textures waiting for upload.
  std::shared_ptr<UploadQueue> upload_queue_;

  // State.
  const std::unique_ptr<SelectorView> selector_view_;

  // Tabs. Public tabs are before private tabs in order.
  std::vector<TabPointer> tabs_;

  // Access to tabs by id.
  std::unordered_map<int, TabPointer> tabs_by_id_;

  // Reverse map with cached index for each tab. Values might be obsolete and
  // need to be validated before being used.
  std::unordered_map<TabPointer, size_t> cached_tab_indices_;

  // Number of public tabs.
  size_t public_tab_count_;

  TabPointer active_tab_;

  bool private_mode_;

  // Thumbnails.
  ThumbnailCache thumbnail_cache_;
  int64_t thumbnail_generation_;
  ThumbnailCache chrome_cache_;
  int64_t chrome_generation_;

  // Input.
  std::unique_ptr<ui::GestureDetector> gesture_detector_;

  void OnGLContextLost();

  // Render resources.
  std::unique_ptr<GLGeometry> image_quad_;
  std::unique_ptr<GLGeometry> outline_quad_;
  std::unique_ptr<GLGeometry> solid_quad_;
  std::unique_ptr<GLShader> image_shader_;
  std::unique_ptr<GLShader> color_shader_;

  bool is_low_res_device_;
  int screen_width_;
  int screen_height_;
  int viewport_top_height_;
  int viewport_bottom_height_;
  int touch_slop_;
  gfx::Vector2dF projection_scale_;

  bool CreateRenderResources();
  void SyncThumbnails();
};

}  // namespace tabui
}  // namespace opera

#endif  // CHILL_BROWSER_UI_TABS_TAB_SELECTOR_RENDERER_H_
