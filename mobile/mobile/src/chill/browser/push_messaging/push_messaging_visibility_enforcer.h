// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software AS

#ifndef CHILL_BROWSER_PUSH_MESSAGING_PUSH_MESSAGING_VISIBILITY_ENFORCER_H_
#define CHILL_BROWSER_PUSH_MESSAGING_PUSH_MESSAGING_VISIBILITY_ENFORCER_H_

#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chill/browser/notifications/opera_platform_notification_service.h"

class GURL;

namespace content {
class BrowserContext;
class PlatformNotificationData;
}

namespace opera {

// Implements a simple policy for enforcing user-visible push messages.
//
// For each handled push message, EnforcePolicyAfterDelay should be called.
// This will wait for a bit, then check that a notification was actually
// displayed. If no notification was seen, we create a generic notification.
class PushMessagingVisibilityEnforcer
    : public OperaPlatformNotificationService::Observer {
 public:
  explicit PushMessagingVisibilityEnforcer(content::BrowserContext*);
  ~PushMessagingVisibilityEnforcer();

  class AutoDecMessages {
   public:
    explicit AutoDecMessages(PushMessagingVisibilityEnforcer* enforcer);
    ~AutoDecMessages();
    void Cancel();

   private:
    PushMessagingVisibilityEnforcer* enforcer_;
  };

  void IncMessages();
  void DecMessages();

  void EnforcePolicyAfterDelay(const GURL& origin,
                               int service_worker_registration_id,
                               AutoDecMessages* auto_dec);

 private:
  // OperaPlatformNotificationService::Observer implementation:
  void OnDisplayNotification(
      const GURL& origin,
      const content::PlatformNotificationData& notification_data) override;

  void EnforcePolicy(const GURL& origin, int service_worker_registration_id);

  static void DidWriteNotificationData(
      const base::WeakPtr<PushMessagingVisibilityEnforcer>& weak_ptr,
      const GURL& origin,
      const content::PlatformNotificationData& notification_data,
      bool success,
      const std::string& notification_id);

  void DisplayGenericNotification(
      const GURL& origin,
      const content::PlatformNotificationData& notification_data,
      bool success,
      const std::string& notification_id);

  content::BrowserContext* context_;
  int messages_;

  base::WeakPtrFactory<PushMessagingVisibilityEnforcer> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PushMessagingVisibilityEnforcer);
};

}  // namespace opera

#endif  // CHILL_BROWSER_PUSH_MESSAGING_PUSH_MESSAGING_VISIBILITY_ENFORCER_H_
