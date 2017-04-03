// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/notifications/notification_bridge.h"
#include "chill/browser/notifications/notification_dispatcher.h"
#include "content/public/browser/desktop_notification_delegate.h"
%}

namespace content {

// The upstream header is simple enough to %include directly, however our
// patched swig messes up on the pure virutals, see WAM-8181. Until that
// is fixed we provide a simplified DesktopNotificationDelegate here.

class DesktopNotificationDelegate {
 public:
  virtual ~DesktopNotificationDelegate();

  virtual void NotificationDisplayed();
  virtual void NotificationClosed();
  virtual void NotificationClick();

 private:
  DesktopNotificationDelegate();
};

JAVA_PROXY_TAKES_OWNERSHIP(DesktopNotificationDelegate);

}  // namespace content

%feature("director", assumeoverride=1) opera::NotificationBridge;
%rename ("NativeNotificationBridge") opera::NotificationBridge;

%include "browser/notifications/notification_bridge.h"
%include "browser/notifications/notification_dispatcher.h"
