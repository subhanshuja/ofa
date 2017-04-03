// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/browser/ui/webui/flags_ui.h
// @last-synchronized ba84619e63fb09545be28988bad3c8ddb5fb6adc

#ifndef CHILL_BROWSER_WEBUI_FLAGS_UI_H_
#define CHILL_BROWSER_WEBUI_FLAGS_UI_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_ui_controller.h"

namespace opera {

class FlagsUI : public content::WebUIController {
 public:
  explicit FlagsUI(content::WebUI* web_ui, const std::string& host);
  virtual ~FlagsUI();

 private:
  base::WeakPtrFactory<FlagsUI> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FlagsUI);
};

}  // namespace opera

#endif  // CHILL_BROWSER_WEBUI_FLAGS_UI_H_
