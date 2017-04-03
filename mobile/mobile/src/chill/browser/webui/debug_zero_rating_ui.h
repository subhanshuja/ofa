// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_WEBUI_DEBUG_ZERO_RATING_UI_H_
#define CHILL_BROWSER_WEBUI_DEBUG_ZERO_RATING_UI_H_

#include <string>

#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"

namespace opera {

class DebugZeroRatingUI : public content::WebUIController {
 public:
  DebugZeroRatingUI(content::WebUI* web_ui, std::string source_name);

 private:
  DISALLOW_COPY_AND_ASSIGN(DebugZeroRatingUI);
};

}  // namespace opera

#endif  // CHILL_BROWSER_WEBUI_DEBUG_ZERO_RATING_UI_H_
