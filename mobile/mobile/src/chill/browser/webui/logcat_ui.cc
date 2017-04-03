// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/webui/logcat_ui.h"

#include <android/log.h>

#include <sstream>

#include "base/memory/ref_counted_memory.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"

#include "chill/common/opera_url_constants.h"

namespace opera {

std::string LogcatUISource::GetSource() const {
  return kOperaLogcatHost;
}

std::string LogcatUISource::GetMimeType(const std::string& path) const {
  return "text/html";
}

std::string LogcatUISource::FormatLogcatLine(const std::string& line) const {
  std::string color;
  char type = line[0];
  switch (type) {
    case 'F':
      color = "#ff1111";  // red
      break;
    case 'D':
      color = "#555555";  // grey
      break;
    case 'W':
      color = "#ffcc00";  // dark yellow
      break;
    case 'I':
      color = "#009900";  // foresty green
      break;
    case 'E':
      color = "#800000";  // brown-red
    default:
      color = "";
  }

  return "<span style=\"color: " + color + "\">" + line + "</span>";
}

void LogcatUISource::CollectLogcat(const OpArguments& args) {
  std::string logcat_output =
      reinterpret_cast<const OpLogcatArguments*>(&args)->results;

  std::ostringstream output_buffer;
  std::istringstream isr(logcat_output);

  // Assume UTF-8
  output_buffer << "<meta charset=\"UTF-8\">\n";
  std::string line;
  while (getline(isr, line)) {
    output_buffer << FormatLogcatLine(line) << "</br>\n";
  }

  std::string output = output_buffer.str();
  callback_.Run(base::RefCountedString::TakeString(&output));
}

void LogcatUISource::StartDataRequest(
    const std::string& path,
    const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
    const content::URLDataSource::GotDataCallback& callback) {
  callback_ = callback;

  // Fetch the last 4096 lines from logcat
  logcat_util_.StartFetchData(
      4096, base::Bind(&LogcatUISource::CollectLogcat, base::Unretained(this)));
}

LogcatUI::LogcatUI(content::WebUI* web_ui, const std::string&)
    : WebUIController(web_ui) {
  content::URLDataSource::Add(web_ui->GetWebContents()->GetBrowserContext(),
                              new LogcatUISource());
}

}  // namespace opera
