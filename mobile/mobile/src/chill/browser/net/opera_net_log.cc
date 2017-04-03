// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/shell_net_log.cc
// @final-synchronized

#include "chill/browser/net/opera_net_log.h"

#include <stdio.h>
#include <string>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "base/files/file_path.h"
#include "base/files/scoped_file.h"
#include "content/public/common/content_switches.h"
#include "content/public/browser/browser_thread.h"
#include "net/log/write_to_file_net_log_observer.h"
#include "net/log/net_log_util.h"

#include "chill/common/opera_switches.h"

#include "common/settings/settings_manager.h"

namespace opera {

OperaNetLog::OperaNetLog() {
  // Must first be created on the UI thread, because of GetOperaConstants()
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();

  net::NetLogCaptureMode base_capture_mode = net::NetLogCaptureMode::IncludeCookiesAndCredentials();
  if (command_line->HasSwitch(switches::kNetLogLevel)) {
    std::string log_level_string =
        command_line->GetSwitchValueASCII(switches::kNetLogLevel);
    int command_line_log_level;
    if (base::StringToInt(log_level_string, &command_line_log_level)) {
      switch(command_line_log_level) {
        case 0: // LOG_ALL
          base_capture_mode = net::NetLogCaptureMode::IncludeSocketBytes();
          break;
        case 1: // LOG_ALL_BUT_BYTES
          base_capture_mode = net::NetLogCaptureMode::IncludeCookiesAndCredentials();
          break;
        case 2: // LOG_STRIP_PRIVATE_DATA
          base_capture_mode = net::NetLogCaptureMode::Default();
          break;
        case 3: // LOG_NONE
        default:
          NOTREACHED();
          break;
      }
    }
  }

  if (command_line->HasSwitch(switches::kLogNetLog)) {
    base::FilePath log_path =
        command_line->GetSwitchValuePath(switches::kLogNetLog);
    // According to ShellNetLog:
    // Much like logging.h, bypass threading restrictions by using fopen
    // directly.  Have to write on a thread that's shutdown to handle events on
    // shutdown properly, and posting events to another thread as they occur
    // would result in an unbounded buffer size, so not much can be gained by
    // doing this on another thread.  It's only used when debugging Chrome, so
    // performance is not a big concern.
    base::ScopedFILE file;
    file.reset(fopen(log_path.value().c_str(), "w"));

    if (!file) {
      LOG(ERROR) << "Could not open file " << log_path.value()
                 << " for net logging";
    } else {
      std::unique_ptr<base::Value> constants(GetOperaConstants());
      net_log_logger_.reset(new net::WriteToFileNetLogObserver());
      net_log_logger_->set_capture_mode(base_capture_mode);
      net_log_logger_->StartObserving(this, std::move(file), constants.get(),
                                      nullptr);
      VLOG(0) << "Prepared OperaNetLog for file=" << log_path.value().c_str();
    }
  }
}

// static
base::Value* OperaNetLog::GetOperaConstants() {
  std::unique_ptr<base::DictionaryValue> constants_dict = net::GetNetConstants();
  DCHECK(constants_dict);

  // Add a dictionary with the version of the client and its command line
  // arguments.
  {
    base::DictionaryValue* dict = new base::DictionaryValue();

    // We have everything we need to send the right values.
    dict->SetString("name", "Opera");
    dict->SetString("version", OPR_VERSION);
    dict->SetString("buildno", OPR_BUILD_NUMBER);
    dict->SetString("chr_version", CHR_VERSION);

    // Below two lines crashes because strings are not UTF8
    // dict->SetString("turbo_client_id",
    //     SettingsManager::GetTurboClientId());
    // dict->SetString("turbo_device_id",
    //     SettingsManager::GetTurboDeviceId());

    dict->SetBoolean("turbo_enabled", SettingsManager::GetCompression());
    dict->SetBoolean("turbo_video_compression_enabled",
        SettingsManager::GetCompression());
    dict->SetInteger("turbo_image_quality",
        static_cast<int>(SettingsManager::GetTurboImageQualityMode()));
    dict->SetString("os_type", "android");
    dict->SetString("command_line",
        base::CommandLine::ForCurrentProcess()->GetCommandLineString());

    constants_dict->Set("clientInfo", dict);
  }

  return constants_dict.release();
}

OperaNetLog::~OperaNetLog() {
  // Remove the observers we own before we're destroyed.
  if (net_log_logger_)
    net_log_logger_->StopObserving(nullptr);
}

}  // namespace opera
