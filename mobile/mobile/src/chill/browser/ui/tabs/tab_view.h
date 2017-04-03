// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_UI_TABS_TAB_VIEW_H_
#define CHILL_BROWSER_UI_TABS_TAB_VIEW_H_

#include <memory>
#include <string>
#include <vector>

#include "ui/gfx/geometry/vector2d_f.h"

#include "common/opengl_utils/vector4d_f.h"

class SkPaint;

namespace opera {

class GLMipMap;

namespace tabui {

class TabImageQuad;
class TabSelectorRenderer;

// A screen shot of a webpage with optional chrome attached.
class TabView {
 public:
  enum VisibleSize { kZeroSize, kSmallSize, kBigSize, kZoomedSize };

  TabView(int id,
          bool is_dashboard,
          bool is_private,
          TabSelectorRenderer* host);
  ~TabView();

  // Return unique tab id.
  int id() const { return id_; }
  bool is_dashboard() const { return is_dashboard_; }
  bool is_private() const { return is_private_; }

  // Access to the underlying screenshot image.
  const std::shared_ptr<GLMipMap> mipmap() const;

  // Set up all render resources needed.
  //
  // screen_shot:     Screen shot of the web page.
  // chrome:          Title bar.
  void Create(const std::shared_ptr<GLMipMap>& screen_shot,
              const std::shared_ptr<GLMipMap>& chrome);

  // Update the screen shot. This should always be done after a frame has
  // finished rendering, never during a frame update.
  void UpdateScreenshot(const std::shared_ptr<GLMipMap>& screen_shot);

  // Update the chrome. This should always be done after a frame has
  // finished rendering, never during a frame update.
  void UpdateChrome(const std::shared_ptr<GLMipMap>& screen_chrome);

  // offset:          The camera position. x will be non zero if we're in
  //                  switch mode or when panned.
  // scale:           How zoomed out we currently are. 2.0 for fullscreen and
  //                  around 1.0 when zoomed out in the selector view.
  // color:           Color and alpha of the image.
  // cropping:        ?
  // chrome_opacity:  Opacity of the chrome, 0.0 means hidden, 1.0 means fully
  //                  visible.
  // dimmed:          The blend value between full color image and a dark shade.
  //                  This is normally 0.0 for regular tabs, 1.0 for private
  //                  ones when they're not active and any value in between
  //                  whenever there's a transition.
  void Draw(gfx::Vector2dF offset,
            gfx::Vector2dF scale,
            gfx::Vector4dF color,
            gfx::Vector2dF cropping,
            float chrome_opacity,
            float dimmed);

  bool IsVisible(gfx::Vector2dF offset, gfx::Vector2dF scale);

  bool CheckHit(gfx::Vector2dF offset, gfx::Vector2dF input);
  bool CheckHitClose(gfx::Vector2dF offset, gfx::Vector2dF input);

  void RequestUpload(VisibleSize visible_size, bool portrait_mode);
  void RequestUpload();

  gfx::Vector2dF Cropping() const;

  static const gfx::Vector2dF GetTabScale() {
    return gfx::Vector2dF(0.5f, 0.5f);
  }
  gfx::Vector2dF GetChromeScale() const;
  gfx::Vector2dF GetChromeOffset(gfx::Vector2dF offset,
                                       gfx::Vector2dF scale) const;
  gfx::Vector2dF GetChromeScale(gfx::Vector2dF scale) const;

 private:
  void RequestUpload(VisibleSize visible_size,
                     bool portrait_mode,
                     const std::shared_ptr<TabImageQuad>& image_quad);

  // Unique id of the tab.
  const int id_;

  // True for dashboard tab, which is not closable and may be drawn a
  // bit differently.
  const bool is_dashboard_;

  // Privacy mode.
  const bool is_private_;

  // Some shared resources are stored in the host object.
  TabSelectorRenderer* host_;

  // The render resources to draw the screenshot with chrome.
  std::shared_ptr<TabImageQuad> screenshot_;
  std::shared_ptr<TabImageQuad> chrome_;

  void GetHitAreas(gfx::Vector2dF* page_offset,
                   gfx::Vector2dF* page_scale,
                   gfx::Vector2dF* chrome_offset,
                   gfx::Vector2dF* chrome_scale);
};

// A tab view and all the attributes needed to draw it.
struct TabViewDrawEntry {
  std::shared_ptr<TabView> tab;
  gfx::Vector2dF offset;
  gfx::Vector2dF scale;
  gfx::Vector4dF color;
  gfx::Vector2dF cropping;
  float chrome_opacity;
  float dimmed;
};

typedef std::vector<TabViewDrawEntry> VisibleTabsContainer;

}  // namespace tabui
}  // namespace opera

#endif  // CHILL_BROWSER_UI_TABS_TAB_VIEW_H_
