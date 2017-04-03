// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SETTINGS_SETTINGS_MANAGER_DELEGATE_H_
#define COMMON_SETTINGS_SETTINGS_MANAGER_DELEGATE_H_

#include <string>

#include "base/android/scoped_java_ref.h"
#include "net/turbo/turbo_constants.h"

namespace opera {

class SettingsManagerDelegate
    : public base::android::ScopedJavaGlobalRef<jobject> {
 public:
  virtual ~SettingsManagerDelegate() {}
  virtual bool GetJavaScript() = 0;
  virtual int GetUserAgentInt() = 0;
  virtual bool GetUseDesktopUserAgent() = 0;
  virtual int GetBlockPopupsInt() = 0;
  virtual int GetCookiesInt() = 0;
  virtual bool GetTextWrap() = 0;
  virtual bool GetForceEnableZoom() = 0;
  virtual bool GetFraudProtection() = 0;
  virtual bool GetReadingMode() = 0;
  virtual bool GetCompression() = 0;
  virtual bool GetAdBlocking() = 0;
  virtual bool GetVideoCompression() = 0;
  virtual net::TurboImageQuality GetTurboImageQualityMode() = 0;
  virtual std::string GetTurboClientId() = 0;
  virtual std::string GetTurboDeviceId() = 0;
  virtual std::string GetTurboSuggestedServer() = 0;
};

}  // namespace opera

#endif  // COMMON_SETTINGS_SETTINGS_MANAGER_DELEGATE_H_
