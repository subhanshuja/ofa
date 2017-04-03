// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_APP_NATIVE_INTERFACE_H_
#define CHILL_APP_NATIVE_INTERFACE_H_

#include "base/android/scoped_java_ref.h"

#include "common/java_bridge/native_interface_base.h"


namespace opera {

class NotificationBridge;
class OperaDownloadManagerDelegate;
class PermissionBridge;
class SettingsManagerDelegate;

class NativeInterface : public base::android::ScopedJavaGlobalRef<jobject>,
                        public NativeInterfaceBase {
 public:
  virtual void RestartBrowser() = 0;
  virtual SettingsManagerDelegate* GetSettingsManagerDelegate() = 0;
  virtual OperaDownloadManagerDelegate* GetDownloadManagerDelegate(
      bool store_downloads) = 0;
  virtual PermissionBridge* GetPermissionBridge(bool private_mode) = 0;
  virtual NotificationBridge* GetNotificationBridge() = 0;

  static NativeInterface* Get();
};

}  // namespace opera

#endif  // CHILL_APP_NATIVE_INTERFACE_H_
