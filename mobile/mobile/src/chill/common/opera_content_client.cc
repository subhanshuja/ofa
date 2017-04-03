// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/shell_content_client.cc
// @final-synchronized

#include "chill/common/opera_content_client.h"

#include "base/command_line.h"
#include "base/strings/string_piece.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/user_agent.h"
#include "jni/DisplayUtil_jni.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

#include "chill/common/opera_switches.h"

namespace opera {

OperaContentClient::~OperaContentClient() {
}

std::string GetProduct() {
  return std::string("OPR/" OPR_VERSION);
}

std::string OperaContentClient::GetProduct() const {
  return opera::GetProduct();
}

std::string GetUserAgent() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  std::string process_type =
      command_line->GetSwitchValueASCII(switches::kProcessType);

  if (command_line->HasSwitch(switches::kUserAgent)) {
    return command_line->GetSwitchValueASCII(switches::kUserAgent);
  } else if (process_type.empty()) {
    std::string chrome_product("Chrome/" CHR_VERSION);
    const bool is_tablet_form_factor = Java_DisplayUtil_isTabletFormFactor(
        base::android::AttachCurrentThread());
    if (!is_tablet_form_factor)
      chrome_product += " Mobile";
    return content::BuildUserAgentFromProduct(chrome_product) + " " +
           GetProduct();
  }

  NOTREACHED() << "User agent must be set on the command line";

  return std::string();
}

std::string OperaContentClient::GetUserAgent() const {
  return opera::GetUserAgent();
}

std::string OperaContentClient::GetUserAgentOverride() const {
  std::string chrome_product("Chrome/" CHR_VERSION);
  return content::BuildUserAgentFromOSAndProduct("X11; Linux x86_64",
                                                 chrome_product) +
         " " + GetProduct();
}

base::string16 OperaContentClient::GetLocalizedString(int message_id) const {
  return l10n_util::GetStringUTF16(message_id);
}

base::StringPiece OperaContentClient::GetDataResource(
    int resource_id,
    ui::ScaleFactor scale_factor) const {
  return ResourceBundle::GetSharedInstance().GetRawDataResourceForScale(
      resource_id, scale_factor);
}

base::RefCountedMemory* OperaContentClient::GetDataResourceBytes(
    int resource_id) const {
  return ResourceBundle::GetSharedInstance().LoadDataResourceBytes(resource_id);
}


}  // namespace opera
