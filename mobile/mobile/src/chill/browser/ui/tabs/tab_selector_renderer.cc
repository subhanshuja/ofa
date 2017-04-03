// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/ui/tabs/tab_selector_renderer.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

#include "base/logging.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "ui/gfx/android/device_display_info.h"
#include "ui/events/gesture_detection/motion_event.h"
#include "ui/events/gesture_detection/motion_event_generic.h"

#include "chill/browser/ui/tabs/interpolator.h"
#include "chill/browser/ui/tabs/selector_view.h"
#include "chill/browser/ui/tabs/shaders.h"
#include "chill/browser/ui/tabs/tab_view.h"
#include "chill/browser/ui/tabs/upload_queue.h"
#include "common/opengl_utils/gl_geometry.h"
#include "common/opengl_utils/gl_library.h"
#include "common/opengl_utils/gl_mipmap.h"
#include "common/opengl_utils/gl_shader.h"

namespace opera {
namespace tabui {

TabSelectorRenderer::TabSelectorRenderer(const ThumbnailCache& thumbnail_cache,
                                         const ThumbnailCache& chrome_cache,
                                         const float title_ratio,
                                         bool is_rtl)
    : is_rtl_(is_rtl),
      initialized_(false),
      hidden_(true),
      first_draw_(true),
      surface_created_(false),
      portrait_mode_(true),
      title_ratio_(title_ratio),
      selector_view_(new SelectorView(this)),
      public_tab_count_(0),
      private_mode_(false),
      thumbnail_cache_(thumbnail_cache),
      thumbnail_generation_(thumbnail_cache.generation()),
      chrome_cache_(chrome_cache),
      chrome_generation_(chrome_cache.generation()),
      gesture_detector_(nullptr),
      is_low_res_device_(IsLowResDevice()),
      screen_width_(0),
      screen_height_(0),
      viewport_top_height_(0),
      viewport_bottom_height_(0),
      projection_scale_(1, 1) {}

TabSelectorRenderer::~TabSelectorRenderer() {}

bool TabSelectorRenderer::IsLowResDevice() {
  const double kLowResDIP = 1.5;  // 240 dpi
  gfx::DeviceDisplayInfo display_info;
  return display_info.GetDIPScale() < kLowResDIP;
}

void TabSelectorRenderer::OnSurfaceCreated() {
  if (!GLLibrary::Get().Init())
    LOG(FATAL) << "Failed to initialize the renderer.";

  if (initialized_) {
    // This means that it wasn't the first time the engine was initialized.
    // That indicates that the device was rotated.
    // The OpenGL context has already been recreated, all currently held
    // resource identifiers are now invalid.
    OnGLContextLost();
  }

  if (!upload_queue_)
    upload_queue_ = UploadQueue::Create();

  if (!CreateRenderResources())
    LOG(FATAL) << "Failed to create render resources for the tab ui.";

  if (!selector_view_->OnCreate())
    LOG(FATAL) << "Failed to create the selector view.";

  if (!initialized_) {
    // Start up an empty scene.
    active_tab_ = 0;
    initialized_ = true;
  }

  start_time_ = base::Time::Now();
}

void TabSelectorRenderer::OnSurfaceChanged(int width, int height) {
  CHECK_NE(viewport_top_height_, 0);

  // We will draw over the whole viewport no matter what.
  GLLibrary& gl = GLLibrary::Get();
  gl.Viewport(width, height);

  screen_width_ = width;
  screen_height_ = height;

  // Set up the projection.
  portrait_mode_ = width < height;
  if (portrait_mode_) {
    // Portrait mode is unaffected by projection scale.
    projection_scale_ = gfx::Vector2dF(1, 1);
  } else {
    // Treat landscape mode as a very wide portrait mode by setting a high
    // horizontal scale factor.
    float aspect_ratio = height / static_cast<float>(width);
    projection_scale_.set_x(aspect_ratio * aspect_ratio);
    projection_scale_.set_y(1);
  }

  if (is_rtl_) {
    projection_scale_.Scale(-1.f, 1.f);
  }

  selector_view_->OnSurfaceChanged(width, height);
}

void TabSelectorRenderer::OnSetViewportLimits(int top_height,
                                              int bottom_height) {
  viewport_top_height_ = top_height;
  viewport_bottom_height_ = bottom_height;
}

void TabSelectorRenderer::OnSetTouchSlop(int touch_slop) {
  touch_slop_ = touch_slop;

  ui::GestureDetector::Config config;
  config.touch_slop = touch_slop;
  // Apparently defaults to 8000, which is too low.
  config.maximum_fling_velocity =
      std::max(config.maximum_fling_velocity, 16000.f);
  gesture_detector_.reset(
      new ui::GestureDetector(config, selector_view_.get(), nullptr));
  gesture_detector_->set_longpress_enabled(false);
  gesture_detector_->set_showpress_enabled(false);
}

void TabSelectorRenderer::OnSetPlaceholderGraphics(jobject bitmap) {
  selector_view_->OnSetPlaceholderGraphics(bitmap);
}

void TabSelectorRenderer::OnDrawFrame() {
  // Keep track of the current time.
  current_time_ = base::Time::Now();

  // Clear the render buffer before we start drawing.
  // We do this even if we're hidden to avoid showing garbage on the first
  // visible frame.
  const float rgba[] = {0, 0, 0, 1};
  GLLibrary& gl = GLLibrary::Get();
  gl.Clear(rgba);

  if (!tabs_.size()) {
    return;
  }

  gl.EnableBlend();

  if (first_draw_) {
    // We need to wait with starting up the animations since they depend on
    // the current time and the init sequence can take a while.
    selector_view_->OnEnter();
    first_draw_ = false;
  }

  SyncThumbnails();
  upload_queue_->OnDrawFrame();

  selector_view_->OnDraw();
}

namespace {
enum InputAction { kDown = 0, kUp = 1, kMove = 2 };

ui::MotionEvent::Action ToAction(int input_action) {
  switch (input_action) {
    case kDown:
      return ui::MotionEvent::ACTION_DOWN;
    case kMove:
      return ui::MotionEvent::ACTION_MOVE;
    case kUp:
      return ui::MotionEvent::ACTION_UP;
    default:
      DCHECK(!"unsupported");
      return ui::MotionEvent::ACTION_NONE;
  }
}

ui::MotionEventGeneric MakeMotionEvent(ui::MotionEvent::Action action,
                                       float x,
                                       float y) {
  DCHECK(action != ui::MotionEvent::ACTION_NONE);
  // TODO(wonko): Consider passing actual value from java, like what's
  // currently done with touch-slop.
  float touch_major = 0.f;
  ui::PointerProperties pointer(x, y, touch_major);
  return ui::MotionEventGeneric(action, base::TimeTicks::Now(), pointer);
}
}  // namespace

gfx::Vector2dF TabSelectorRenderer::ToRelativeCoord(const ui::MotionEvent& e) {
  // Normalize the coordinates.
  //
  // The input coordinates we get are relative to the GLSurfaceView, that means
  // that we'll sometimes get coordinates that are outside (0,0 .. 1,1), i.e.
  // the status bar on top or button-row/system menu on the bottom.
  return gfx::Vector2dF(e.GetX() / screen_width_,
                        e.GetY() / screen_height_);
}

void TabSelectorRenderer::OnTouchEvent(int action, float x, float y) {
  DCHECK(gesture_detector_);

  ui::MotionEventGeneric e = MakeMotionEvent(ToAction(action), x, y);

  bool handled = gesture_detector_->OnTouchEvent(e);

  if (action == kUp) {
    // Extended GestureListener interface for
    // TabSelectorRenderer->SelectorView interaction: always call OnUp
    // on up, and additionally OnDragDone if no gesture was triggered.
    selector_view_->OnUp(e);
    if (!handled) {
      handled = selector_view_->OnDragDone(e);
    }
  }
}

void TabSelectorRenderer::OnGLContextLost() {
  if (image_quad_)
    image_quad_->OnGLContextLost();
  if (outline_quad_)
    outline_quad_->OnGLContextLost();
  if (solid_quad_)
    solid_quad_->OnGLContextLost();
  if (image_shader_)
    image_shader_->OnGLContextLost();
  if (color_shader_)
    color_shader_->OnGLContextLost();

  selector_view_->OnGLContextLost();
}

void TabSelectorRenderer::SwitchToPageView() {
  DoZoomedIn();
}

int TabSelectorRenderer::GetIndexOf(const TabPointer& tab) {
  auto it = cached_tab_indices_.find(tab);
  if (it == cached_tab_indices_.end()) {
    // A tab can not be in the vector if not in the cache.
    return -1;
  }
  // Cached index is invalid if not same key is found at cached position.
  if (it->second >= tabs_.size() || tabs_[it->second] != tab) {
    // Do a linear search to find correct index and update cache.
    it->second = distance(tabs_.begin(), find(tabs_.begin(), tabs_.end(), tab));
  }
  return it->second;
}

int TabSelectorRenderer::Distance(const TabPointer& first,
                                  const TabPointer& last) {
  int first_ix = GetIndexOf(first);
  int last_ix = GetIndexOf(last);
  CHECK_NE(first_ix, -1);
  CHECK_NE(last_ix, -1);
  return last_ix - first_ix;
}

void TabSelectorRenderer::RemoveTab(const TabPointer& tab) {
  int index = GetIndexOf(tab);
  CHECK_NE(index, -1);

  OnTabRemoved(tab->id());
  DoRemoveTab(tab->id());
}

void TabSelectorRenderer::SelectTab(int id) {
  auto it = tabs_by_id_.find(id);
  DCHECK(it != tabs_by_id_.end());
  TabPointer tab = it->second;
  DCHECK(tab);
  selector_view_->SelectTab(tab);
}

void TabSelectorRenderer::SetActiveTab(const TabPointer& tab) {
  DCHECK(tab);
  DoSetActiveTab(tab->id());
}

// -- Interface with the Java code.

void TabSelectorRenderer::OnTabsSetup(int index,
                                      int id,
                                      bool is_dashboard,
                                      bool is_private) {
  DCHECK_GE(index, 0);
  DCHECK(is_private || (index <= static_cast<int>(public_tab_count_)));
  DCHECK(!is_private || (index >= static_cast<int>(public_tab_count_)));

  TabPointer tab =
      std::make_shared<TabView>(id, is_dashboard, is_private, this);
  tab->Create(thumbnail_cache_.GetMipmap(id), chrome_cache_.GetMipmap(id));
  tabs_.insert(tabs_.begin() + index, tab);
  tabs_by_id_[id] = tab;
  cached_tab_indices_[tab] = index;
  if (!is_private)
    ++public_tab_count_;
  DCHECK_EQ(tabs_.size(), tabs_by_id_.size());
  DCHECK_EQ(tabs_.size(), cached_tab_indices_.size());
}

// Insert new tab as first standard or private tab.
void TabSelectorRenderer::OnTabsSetup(int id,
                                      bool is_dashboard,
                                      bool is_private) {
  int index = is_private ? tabs_.size() : public_tab_count_;
  OnTabsSetup(index, id, is_dashboard, is_private);
}

void TabSelectorRenderer::OnTabsSetup(int id,
                                      bool is_dashboard,
                                      bool is_private,
                                      int after_id) {
  auto it = tabs_by_id_.find(after_id);
  if (it == tabs_by_id_.end() || it->second->is_private() != is_private) {
    OnTabsSetup(id, is_dashboard, is_private);
  } else {
    int after_index = GetIndexOf(it->second);
    DCHECK_NE(after_index, -1);
    OnTabsSetup(after_index + 1, id, is_dashboard, is_private);
  }
}

void TabSelectorRenderer::OnTabAdded(int id,
                                     bool is_dashboard,
                                     bool is_private) {
  OnTabsSetup(id, is_dashboard, is_private);
  selector_view_->OnTabAdded(tabs_by_id_[id]);
}

void TabSelectorRenderer::OnTabAdded(int id,
                                     bool is_dashboard,
                                     bool is_private,
                                     int after_id) {
  OnTabsSetup(id, is_dashboard, is_private, after_id);
  selector_view_->OnTabAdded(tabs_by_id_[id]);
}

void TabSelectorRenderer::OnTabRemoved(int id) {
  auto it = tabs_by_id_.find(id);
  if (it == tabs_by_id_.end()) {
    return;
  }
  auto tab = it->second;
  int index = GetIndexOf(tab);
  DCHECK_NE(index, -1);

  // Give the view a chance to adjust focus before we update the tab counts.
  selector_view_->OnTabRemoved(tab);

  tabs_.erase(tabs_.begin() + index);
  tabs_by_id_.erase(it);
  cached_tab_indices_.erase(tab);
  if (active_tab_ == tab) {
    active_tab_.reset();
  }
  if (!tab->is_private())
    --public_tab_count_;
  DCHECK_EQ(tabs_.size(), tabs_by_id_.size());
  DCHECK_EQ(tabs_.size(), cached_tab_indices_.size());
}

void TabSelectorRenderer::OnActiveTabChanged(int id) {
  active_tab_.reset();
  auto it = tabs_by_id_.find(id);
  if (it != tabs_by_id_.end())
    active_tab_ = it->second;
}

void TabSelectorRenderer::OnShow() {
  first_draw_ = true;
  SyncThumbnails();
  if (active_tab_)
    active_tab_->RequestUpload();
}

void TabSelectorRenderer::OnHide() {
  selector_view_->OnHide();
}

void TabSelectorRenderer::OnSetPrivateMode(bool enable) {
  if (enable != private_mode_) {
    private_mode_ = enable;

    // Set up animation to scroll over to the other side.
    selector_view_->ScrollToPrivateMode(enable);
  }
}

int TabSelectorRenderer::GetFocusedTabId() const {
  // Note that active_tab is the last known activated chromium tab, but
  // focused tab is whichever tab is centered in the selector right now.
  if (tabs_.empty())
    return 0;

  int focused_index = selector_view_->GetFocusedIndex();
  if (focused_index < 0)
    return tabs_.front()->id();

  if (static_cast<size_t>(focused_index) >= tabs_.size())
    return tabs_.back()->id();

  return tabs_[focused_index]->id();
}

void TabSelectorRenderer::UpdatePrivateMode(bool enable) {
  if (enable != private_mode_) {
    private_mode_ = enable;

    // Update the Java UI.
    DoSetPrivateMode(enable);
  }
}

bool TabSelectorRenderer::CreateRenderResources() {
  do {
    // Create the shader we can reuse for tabs that are shown with images.
    const char* kImageQuadVertexDescription = "p2f;t2f";
    const char* imageFragmentShader =
        GLLibrary::Get().SupportsShaderTextureLOD() ? kImageFragmentShaderLOD
                                                    : kImageFragmentShader;
    image_shader_ = GLShader::Create(kImageVertexShader, imageFragmentShader,
                                     kImageQuadVertexDescription);
    if (!image_shader_) {
      LOG(ERROR) << "Could not create image shader.";
      break;
    }

    // Create the quad geometry we can reuse for tabs.
    // This creates a normalized unit sized quad with texture coordinates.
    const float kImageQuadVertices[] = {-0.5f, -0.5f, 0.0f, 1.0f,
                                         0.5f, -0.5f, 1.0f, 1.0f,
                                        -0.5f,  0.5f, 0.0f, 0.0f,
                                         0.5f,  0.5f, 1.0f, 0.0f};
    image_quad_ = GLGeometry::Create(
        GL_TRIANGLE_STRIP, kImageQuadVertexDescription, 4, kImageQuadVertices);
    if (!image_quad_) {
      LOG(ERROR) << "Could not create quad geometry.";
      break;
    }

    // Create the shader we can reuse for single colored shapes.
    const char* kSolidColorQuadVertexDescription = "p2f";
    color_shader_ = GLShader::Create(kColorVertexShader, kColorFragmentShader,
                                     kSolidColorQuadVertexDescription);
    if (!color_shader_) {
      LOG(ERROR) << "Could not create color shader.";
      break;
    }

    // Create an outline based on four equally spaced separate lines per side.
    // Each end point of the line segments are spaced (1 / 7) units apart.
    //
    //               (0.5, 0.5)
    //      --  --  --  -*
    //      |            |
    //
    //      |            |
    //
    //      |            |
    //
    //      |            |
    //      *-  --  --  --
    // (-0.5, -0.5)
    //
    float outline_vertices[8 * 4 * 2];
    memset(outline_vertices, 0, 8 * 4 * 2 * sizeof(float));
    for (int i = 0; i < 8; ++i) {
      const float kStep = 1.0f / 7;

      const int kOffsetTop = 0 * 8 * 2;
      outline_vertices[kOffsetTop + i * 2 + 0] = -0.5f + kStep * i;
      outline_vertices[kOffsetTop + i * 2 + 1] = -0.5f;

      const int kOffsetRight = 1 * 8 * 2;
      outline_vertices[kOffsetRight + i * 2 + 0] = 0.5f;
      outline_vertices[kOffsetRight + i * 2 + 1] = -0.5f + kStep * i;

      const int kOffsetBottom = 2 * 8 * 2;
      outline_vertices[kOffsetBottom + i * 2 + 0] = 0.5f - kStep * i;
      outline_vertices[kOffsetBottom + i * 2 + 1] = 0.5f;

      const int kOffsetLeft = 3 * 8 * 2;
      outline_vertices[kOffsetLeft + i * 2 + 0] = -0.5f;
      outline_vertices[kOffsetLeft + i * 2 + 1] = 0.5f - kStep * i;
    }
    outline_quad_ = GLGeometry::Create(
        GL_LINES, kSolidColorQuadVertexDescription, 8 * 4, outline_vertices);
    if (!outline_quad_) {
      LOG(ERROR) << "Could not create outline quad geometry.";
      break;
    }

    // Create the quad geometry we can reuse for tabs.
    // This creates a normalized unit sized quad.
    const float kSolidQuadVertices[] = {-0.5f, -0.5f,
                                         0.5f, -0.5f,
                                        -0.5f,  0.5f,
                                         0.5f,  0.5f};
    solid_quad_ =
        GLGeometry::Create(GL_TRIANGLE_STRIP, kSolidColorQuadVertexDescription,
                           4, kSolidQuadVertices);
    if (!solid_quad_) {
      LOG(ERROR) << "Could not create quad geometry.";
      break;
    }

    return true;
  } while (false);

  // It's all or nothing.
  image_shader_.reset();
  color_shader_.reset();
  image_quad_.reset();
  outline_quad_.reset();
  solid_quad_.reset();
  return false;
}

void TabSelectorRenderer::SyncThumbnails() {
  for (auto p : thumbnail_cache_.GetChangedTabIds(thumbnail_generation_)) {
    auto tab_it = tabs_by_id_.find(p.second);
    if (tab_it != tabs_by_id_.end())
      tab_it->second->UpdateScreenshot(thumbnail_cache_.GetMipmap(p.second));
    thumbnail_generation_ = p.first;
  }
  for (auto p : chrome_cache_.GetChangedTabIds(chrome_generation_)) {
    auto tab_it = tabs_by_id_.find(p.second);
    if (tab_it != tabs_by_id_.end())
      tab_it->second->UpdateChrome(chrome_cache_.GetMipmap(p.second));
    chrome_generation_ = p.first;
  }
}

float TabSelectorRenderer::viewport_height_scale() const {
  return 1.0f -
         (viewport_top_height_ + viewport_bottom_height_) /
             static_cast<float>(screen_height_);
}

}  // namespace tabui
}  // namespace opera
