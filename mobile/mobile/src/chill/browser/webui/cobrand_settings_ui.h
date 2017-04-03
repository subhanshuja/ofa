// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_WEBUI_COBRAND_SETTINGS_UI_H_
#define CHILL_BROWSER_WEBUI_COBRAND_SETTINGS_UI_H_

#include <string>

#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"

namespace opera {

class CobrandSettingsUI : public content::WebUIController {
 public:
  explicit CobrandSettingsUI(content::WebUI* web_ui, std::string source_name);

 private:
  DISALLOW_COPY_AND_ASSIGN(CobrandSettingsUI);
};

}  // namespace opera

#endif  // CHILL_BROWSER_WEBUI_COBRAND_SETTINGS_UI_H_
