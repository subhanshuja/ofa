// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_NOTIFICATIONS_NOTIFICATION_DISPATCHER_H_
#define CHILL_BROWSER_NOTIFICATIONS_NOTIFICATION_DISPATCHER_H_

#include <jni.h>
#include <stdint.h>
#include <string>

#include "base/macros.h"

namespace opera {

class NotificationDispatcher {
 public:
  static void OnClick(
      bool private_mode,
      const std::string& notification_id,
      const std::string& origin,
      int action_index,
      jobject callback);

  static void OnClose(
      bool private_mode,
      const std::string& notification_id,
      const std::string& origin,
      bool by_user,
      jobject callback);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(NotificationDispatcher);
};

}  // namespace opera

#endif  // CHILL_BROWSER_NOTIFICATIONS_NOTIFICATION_DISPATCHER_H_
