// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_NOTIFICATIONS_NOTIFICATION_BRIDGE_H_
#define CHILL_BROWSER_NOTIFICATIONS_NOTIFICATION_BRIDGE_H_

#include <string>

#include "base/strings/string16.h"

namespace content {
class DesktopNotificationDelegate;
}

namespace opera {

class NotificationBridge {
 public:
  virtual ~NotificationBridge() {}

  // Takes ownership of delegate.
  virtual void DisplayNotification(
      const std::string& origin,
      bool private_mode,
      const std::string& tag,
      const base::string16& title,
      const base::string16& body,
      jobject icon,
      int64_t timestamp,
      bool silent,
      jintArray vibration_pattern,
      jobjectArray action_titles,
      bool is_persistent,
      const std::string& notification_id,
      content::DesktopNotificationDelegate* delegate) = 0;

  virtual void CloseNotification(const std::string& notification_id) = 0;

  virtual void ClosePersistentNotification(
      const std::string& notification_id) = 0;

  virtual void DispatchDone(jobject callback) = 0;
};

}  // namespace opera

#endif  // CHILL_BROWSER_NOTIFICATIONS_NOTIFICATION_BRIDGE_H_
