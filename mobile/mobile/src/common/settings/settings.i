// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "common/settings/settings_manager_delegate.h"
%}

%include "src/common/swig_utils/typemaps.i"

%feature("director", assumeoverride=1) opera::SettingsManagerDelegate;
SWIG_SELFREF_NAMESPACED_CONSTRUCTOR(opera, SettingsManagerDelegate);

namespace opera {

class SettingsManagerDelegate {
 public:
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

  virtual ~SettingsManagerDelegate() {};
};

}  // namespace opera
