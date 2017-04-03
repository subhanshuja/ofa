// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/browser/ui/webui/version_ui.cc
// @last-synchronized bfbeec2b3ed31752c6577365fe4a5cb1fdcaba71

#include "chill/browser/webui/version_ui.h"

#include <string>
#include <vector>

#include "base/android/build_info.h"
#include "base/command_line.h"
#include "base/sys_info.h"
#include "base/strings/stringprintf.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/user_agent.h"
#include "grit/wam_resources.h"
#include "opera_common/grit/product_free_common_strings.h"
#include "opera_common/grit/product_related_common_strings.h"
#include "v8/include/v8.h"

#include "chill/common/opera_content_client.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace opera {

namespace {

// Copied from chromium/src/chrome/browser/ui/android/android_about_app_info.cc
std::string GetOsInfo() {
  std::string android_info_str;

  // Append information about the OS version.
  int32_t os_major_version = 0;
  int32_t os_minor_version = 0;
  int32_t os_bugfix_version = 0;
  base::SysInfo::OperatingSystemVersionNumbers(&os_major_version,
                                               &os_minor_version,
                                               &os_bugfix_version);
  base::StringAppendF(&android_info_str, "%d.%d.%d", os_major_version,
                      os_minor_version, os_bugfix_version);

  // Append information about the device.
  bool semicolon_inserted = false;
  std::string android_build_codename = base::SysInfo::GetAndroidBuildCodename();
  std::string android_device_name = base::SysInfo::HardwareModelName();
  if ("REL" == android_build_codename && android_device_name.size() > 0) {
    android_info_str += "; " + android_device_name;
    semicolon_inserted = true;
  }

  // Append the build ID.
  std::string android_build_id = base::SysInfo::GetAndroidBuildID();
  if (android_build_id.size() > 0) {
    if (!semicolon_inserted) {
      android_info_str += ";";
    }
    android_info_str += " Build/" + android_build_id;
  }

  return android_info_str;
}

content::WebUIDataSource* CreateVersionUIDataSource(const std::string& host) {
  content::WebUIDataSource* html_source =
      content::WebUIDataSource::Create(host);

  // Localized and data strings.
  html_source->AddLocalizedString("title", IDS_ABOUT_VERSION_TITLE);
  html_source->AddLocalizedString("os_name", IDS_ABOUT_VERSION_OS);
  html_source->AddString("os_type", "Android");
  html_source->AddString("chromium_version", CHR_VERSION " (@" LASTCHANGE ")");
  html_source->AddString("blink_version", content::GetWebKitVersion());
  html_source->AddString("js_engine", "V8");
  html_source->AddString("js_version", v8::V8::GetVersion());

  html_source->AddLocalizedString("application_label",
                                  IDS_ABOUT_VERSION_APPLICATION);
  html_source->AddString("os_version", GetOsInfo());
  base::android::BuildInfo* android_build_info =
      base::android::BuildInfo::GetInstance();
  html_source->AddString("application_name",
                         android_build_info->package_label());
  html_source->AddString("application_version_name",
                         android_build_info->package_version_name());
  html_source->AddString("application_version_code",
                         android_build_info->package_version_code());
  html_source->AddLocalizedString("build_id_name",
                                  IDS_ABOUT_VERSION_BUILD_ID);

  html_source->AddLocalizedString("user_agent_name",
                                  IDS_ABOUT_VERSION_USER_AGENT);
  html_source->AddString("useragent", GetUserAgent());
  html_source->AddLocalizedString("command_line_name",
                                  IDS_ABOUT_VERSION_COMMAND_LINE);

  std::string command_line;
  typedef std::vector<std::string> ArgvList;
  const ArgvList& argv = base::CommandLine::ForCurrentProcess()->argv();
  for (ArgvList::const_iterator iter = argv.begin(); iter != argv.end(); iter++)
    command_line += " " + *iter;
  // TODO(viettrungluu): |command_line| could really have any encoding, whereas
  // below we assumes it's UTF-8.
  html_source->AddString("command_line", command_line);

  html_source->SetJsonPath("strings.js");
  html_source->AddResourcePath("about_version.css", IDR_ABOUT_VERSION_CSS);
  html_source->SetDefaultResource(IDR_ABOUT_VERSION_HTML);
  return html_source;
}

}  // namespace

VersionUI::VersionUI(content::WebUI* web_ui, const std::string& host)
    : content::WebUIController(web_ui) {

  content::BrowserContext* context =
      web_ui->GetWebContents()->GetBrowserContext();

  content::WebUIDataSource::Add(context, CreateVersionUIDataSource(host));
}

VersionUI::~VersionUI() {
}

}  // namespace opera
