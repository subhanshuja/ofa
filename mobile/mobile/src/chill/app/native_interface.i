// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/app/native_interface.h"
%}

%include "src/common/swig_utils/typemaps.i"

%feature("director", assumeoverride=1) opera::NativeInterface;
SWIG_SELFREF_NAMESPACED_CONSTRUCTOR(opera, NativeInterface);

%warnfilter(473) opera::NativeInterface;

namespace opera {

class OperaDownloadManagerDelegate;
class PermissionBridge;
class NotificationBridge;

class NativeInterface : public NativeInterfaceBase {
 public:
  // Before adding another method to the interface, first consider if it might
  // deserve its own interface and lifetime, and only add a getter for the new
  // interface.
  virtual void RestartBrowser() = 0;
  virtual SettingsManagerDelegate* GetSettingsManagerDelegate() = 0;
  virtual OperaDownloadManagerDelegate* GetDownloadManagerDelegate(
      bool store_downloads) = 0;
  virtual PermissionBridge* GetPermissionBridge(bool private_mode) = 0;
  virtual NotificationBridge* GetNotificationBridge() = 0;

  virtual ~NativeInterface() { }
};

}  // namespace opera
