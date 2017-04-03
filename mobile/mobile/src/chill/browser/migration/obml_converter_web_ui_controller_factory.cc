// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/migration/obml_converter_web_ui_controller_factory.h"

#include <string>

#include "content/public/browser/web_ui.h"
#include "content/public/common/url_constants.h"
#include "extensions/common/constants.h"
#include "url/gurl.h"

#include "chill/common/opera_url_constants.h"
#include "chill/browser/migration/obml_converter_web_ui_controller.h"

namespace opera {

OBMLConverterWebUIControllerFactory::OBMLConverterWebUIControllerFactory(
    GURL url,
    const content::WebContents* web_contents,
    const std::vector<OBMLFontInfo>* font_info,
    base::FilePath conversion_target_directory)
    : url_(url),
      web_contents_(web_contents),
      font_info_(font_info),
      conversion_target_directory_(conversion_target_directory) {
}

content::WebUI::TypeID OBMLConverterWebUIControllerFactory::GetWebUIType(
    content::BrowserContext* browser_context,
    const GURL& url) const {
  static const int64_t kWebUITypeID = 0xabbababba;
  return UseWebUIForURL(browser_context, url)
             ? reinterpret_cast<content::WebUI::TypeID>(kWebUITypeID)
             : content::WebUI::kNoWebUI;
}

bool OBMLConverterWebUIControllerFactory::UseWebUIForURL(
    content::BrowserContext* browser_context,
    const GURL& url) const {
  return !IsDisabled() && url == url_;
}

bool OBMLConverterWebUIControllerFactory::UseWebUIBindingsForURL(
    content::BrowserContext* browser_context,
    const GURL& url) const {
  return !IsDisabled() && UseWebUIForURL(browser_context, url);
}

content::WebUIController*
OBMLConverterWebUIControllerFactory::CreateWebUIControllerForURL(
    content::WebUI* web_ui,
    const GURL& url) const {
  if (web_ui->GetWebContents() != web_contents_)
    return nullptr;

  if (!UseWebUIForURL(nullptr, url))
    return nullptr;

  return new OBMLConverterWebUIController(web_ui, url.host(), font_info_,
                                          conversion_target_directory_);
}

void OBMLConverterWebUIControllerFactory::Disable() {
  web_contents_ = nullptr;
  font_info_ = nullptr;
}

bool OBMLConverterWebUIControllerFactory::IsDisabled() const {
  return !web_contents_;
}

}  // namespace opera
