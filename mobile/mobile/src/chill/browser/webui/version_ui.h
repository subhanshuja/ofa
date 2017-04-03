// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/browser/ui/webui/version_ui.h
// @last-synchronized 74c0c0bc7a5a8c2699c740a1f5dd910a8e2d0ea7

#ifndef CHILL_BROWSER_WEBUI_VERSION_UI_H_
#define CHILL_BROWSER_WEBUI_VERSION_UI_H_

#include <string>

#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"

namespace opera {

// The WebUI handler for chrome://version.
class VersionUI : public content::WebUIController {
 public:
  explicit VersionUI(content::WebUI* web_ui, const std::string& host);
  virtual ~VersionUI();
 private:
  DISALLOW_COPY_AND_ASSIGN(VersionUI);
};

}  // namespace opera

#endif  // CHILL_BROWSER_WEBUI_VERSION_UI_H_
