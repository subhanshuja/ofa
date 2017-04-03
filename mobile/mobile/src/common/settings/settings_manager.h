// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SETTINGS_SETTINGS_MANAGER_H_
#define COMMON_SETTINGS_SETTINGS_MANAGER_H_

#include <string>

#include "base/macros.h"
#include "net/turbo/turbo_constants.h"

namespace opera {
class SettingsManagerDelegate;

class SettingsManager {
 public:
  static void SetDelegate(opera::SettingsManagerDelegate*);

  enum BlockPopups {
    OFF = 0,
    ON
  };

  enum Cookies {
    DISABLED = 0,
    ENABLED,
    ENABLED_NO_THIRD_PARTY,
  };

  // Methods for accessing settings in the SettingsManager instance
  static bool GetJavaScript();
  static bool GetUseDesktopUserAgent();
  static BlockPopups GetBlockPopups();
  static Cookies GetCookies();
  static bool GetTextWrap();
  static bool GetForceEnableZoom();
  static bool GetFraudProtection();
  static bool GetReadingMode();
  static bool GetCompression();
  static bool GetAdBlocking();
  static bool GetVideoCompression();
  static net::TurboImageQuality GetTurboImageQualityMode();

  static std::string GetTurboClientId();
  static std::string GetTurboDeviceId();
  static std::string GetTurboSuggestedServer();

 private:
  static opera::SettingsManagerDelegate* delegate_;
  SettingsManager() {}
  DISALLOW_COPY_AND_ASSIGN(SettingsManager);
};

}  // namespace opera

#endif  // COMMON_SETTINGS_SETTINGS_MANAGER_H_
