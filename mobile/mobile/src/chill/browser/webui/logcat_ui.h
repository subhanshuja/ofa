// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_WEBUI_LOGCAT_UI_H_
#define CHILL_BROWSER_WEBUI_LOGCAT_UI_H_

#include <string>

#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_ui_controller.h"

#include "chill/common/logcat_util.h"
#include "common/swig_utils/op_arguments.h"

namespace opera {

class LogcatUISource : public content::URLDataSource {
 public:
  LogcatUISource() {}

  void StartDataRequest(
      const std::string& path,
      const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
      const content::URLDataSource::GotDataCallback& callback) override;

  std::string GetSource() const override;
  std::string GetMimeType(const std::string& path) const override;

 private:
  LogcatUtil logcat_util_;
  content::URLDataSource::GotDataCallback callback_;

  virtual ~LogcatUISource() {}

  std::string FormatLogcatLine(const std::string& line) const;
  void CollectLogcat(const OpArguments& args);

  DISALLOW_COPY_AND_ASSIGN(LogcatUISource);
};

class LogcatUI : public content::WebUIController {
 public:
  LogcatUI(content::WebUI* web_ui, const std::string&);
  virtual ~LogcatUI() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(LogcatUI);
};

}  // namespace opera

#endif  // CHILL_BROWSER_WEBUI_LOGCAT_UI_H_
