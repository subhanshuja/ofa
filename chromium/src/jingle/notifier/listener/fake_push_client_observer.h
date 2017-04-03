// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef JINGLE_NOTIFIER_LISTENER_NON_BLOCKING_FAKE_PUSH_CLIENT_OBSERVER_H_
#define JINGLE_NOTIFIER_LISTENER_NON_BLOCKING_FAKE_PUSH_CLIENT_OBSERVER_H_

#include "base/compiler_specific.h"
#include "jingle/notifier/listener/push_client_observer.h"

namespace notifier {

// PushClientObserver implementation that can be used for testing.
class FakePushClientObserver : public PushClientObserver {
 public:
  FakePushClientObserver();
  ~FakePushClientObserver() override;

  // PushClientObserver implementation.
  void OnNotificationsEnabled() override;
#if defined(OPERA_SYNC)
  void OnNotificationsDisabled(
      OperaNotificationsDisabledReason reason) override;
#else
  void OnNotificationsDisabled(NotificationsDisabledReason reason) override;
#endif  // OPERA_SYNC
  void OnIncomingNotification(const Notification& notification) override;

#if defined(OPERA_SYNC)
  OperaNotificationsDisabledReason last_notifications_disabled_reason() const;
#else
  NotificationsDisabledReason last_notifications_disabled_reason() const;
#endif  // OPERA_SYNC
  const Notification& last_incoming_notification() const;

 private:
#if defined(OPERA_SYNC)
  OperaNotificationsDisabledReason last_notifications_disabled_reason_;
#else
  NotificationsDisabledReason last_notifications_disabled_reason_;
#endif  // OPERA_SYNC
  Notification last_incoming_notification_;
};

}  // namespace notifier

#endif  // JINGLE_NOTIFIER_LISTENER_NON_BLOCKING_FAKE_PUSH_CLIENT_OBSERVER_H_
