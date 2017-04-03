// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software AS
// @copied-from chromium/src/browser/push_messaging/push_messaging_constants.h
// @final-synchronized

#ifndef CHILL_BROWSER_PUSH_MESSAGING_PUSH_MESSAGING_CONSTANTS_H_
#define CHILL_BROWSER_PUSH_MESSAGING_PUSH_MESSAGING_CONSTANTS_H_

namespace opera {

extern const char kPushMessagingGcmEndpoint[];
extern const char kPushMessagingPushProtocolEndpoint[];

// The tag of the notification that will be automatically shown if a webapp
// receives a push message then fails to show a notification.
extern const char kPushMessagingForcedNotificationTag[];

}  // namespace opera

#endif  // CHILL_BROWSER_PUSH_MESSAGING_PUSH_MESSAGING_CONSTANTS_H_
