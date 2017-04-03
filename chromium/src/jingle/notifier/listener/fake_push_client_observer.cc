// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "jingle/notifier/listener/fake_push_client_observer.h"

namespace notifier {

FakePushClientObserver::FakePushClientObserver()
    :last_notifications_disabled_reason_(DEFAULT_NOTIFICATION_ERROR) {}

FakePushClientObserver::~FakePushClientObserver() {}

void FakePushClientObserver::OnNotificationsEnabled() {
#if defined(OPERA_SYNC)
  last_notifications_disabled_reason_ =
      OperaNotificationsDisabledReason(NO_NOTIFICATION_ERROR);
#else
  last_notifications_disabled_reason_ = NO_NOTIFICATION_ERROR;
#endif  // OPERA_SYNC
}


void FakePushClientObserver::OnNotificationsDisabled(
#if defined(OPERA_SYNC)
    OperaNotificationsDisabledReason reason) {
#else
    NotificationsDisabledReason reason) {
#endif  // OPERA_SYNC
  last_notifications_disabled_reason_ = reason;
}

void FakePushClientObserver::OnIncomingNotification(
    const Notification& notification) {
  last_incoming_notification_ = notification;
}

#if defined(OPERA_SYNC)
OperaNotificationsDisabledReason
#else
NotificationsDisabledReason
#endif  // OPERA_SYNC
FakePushClientObserver::last_notifications_disabled_reason() const {
  return last_notifications_disabled_reason_;
}

const Notification&
FakePushClientObserver::last_incoming_notification() const {
  return last_incoming_notification_;
}

}  // namespace notifier

