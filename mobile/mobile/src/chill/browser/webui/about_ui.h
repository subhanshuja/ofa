// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_WEBUI_ABOUT_UI_H_
#define CHILL_BROWSER_WEBUI_ABOUT_UI_H_

#include <string>

#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_ui_controller.h"
#include "ui/base/layout.h"

// opera:about

namespace opera {

class AboutUISource : public content::URLDataSource {
 public:
  AboutUISource() {}
  virtual void StartDataRequest(
      const std::string& path,
      const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
      const content::URLDataSource::GotDataCallback& callback) override;
  std::string GetSource() const override;
  std::string GetMimeType(const std::string& path) const override;

 private:
  virtual ~AboutUISource() {}
  DISALLOW_COPY_AND_ASSIGN(AboutUISource);
};

class AboutUI : public content::WebUIController {
 public:
  AboutUI(content::WebUI* web_ui, const std::string&);
  virtual ~AboutUI() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(AboutUI);
};

}  // namespace opera

#endif  // CHILL_BROWSER_WEBUI_ABOUT_UI_H_
