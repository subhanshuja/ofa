// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/ui/tabs/tab_selector_renderer.h"
%}

%typemap(jstype) jobject bitmap "android.graphics.Bitmap"
%typemap(jtype) jobject bitmap "android.graphics.Bitmap"

namespace opera {
namespace tabui {

%nodefaultctor TabSelectorRenderer;
%feature ("director", assumeoverride=1) TabSelectorRenderer;

// Apply mixedCase to method names in Java.
%rename("%(regex:/TabSelectorRenderer::([A-Z].*)/\\l\\1/)s", regextarget=1, fullname=1, %$isfunction) ".*";

class TabSelectorRenderer {
 public:
  TabSelectorRenderer(const opera::tabui::ThumbnailCache& thumbnail_cache,
                      const opera::tabui::ThumbnailCache& chrome_cache,
                      const float title_ratio,
                      bool is_rtl);
  virtual ~TabSelectorRenderer();

  void OnDrawFrame();
  void OnSurfaceChanged(int width, int height);
  void OnSurfaceCreated();
  void OnSetViewportLimits(int top_height, int bottom_height);
  void OnSetTouchSlop(int touch_slop);
  void OnSetPlaceholderGraphics(jobject bitmap);
  void OnTouchEvent(int action, float x, float y);

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

  virtual void DoZoomedIn() = 0;
  virtual void DoRemoveTab(int id) = 0;
  virtual void DoSetActiveTab(int id) = 0;
  virtual void DoSetPrivateMode(bool private_mode) = 0;
};

}  // namespace tabui
}  // namespace opera
