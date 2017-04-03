// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software AS
// @copied-from chromium/src/browser/push_messaging/push_messaging_constants.cc
// @final-synchronized

#include "chill/browser/push_messaging/push_messaging_constants.h"

namespace opera {

const char kPushMessagingGcmEndpoint[] =
    "https://android.googleapis.com/gcm/send/";

const char kPushMessagingPushProtocolEndpoint[] =
    "https://fcm.googleapis.com/fcm/send/";

const char kPushMessagingForcedNotificationTag[] =
    "user_visible_auto_notification";

}  // namespace opera
