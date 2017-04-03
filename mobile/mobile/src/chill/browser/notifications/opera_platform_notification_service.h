// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_NOTIFICATIONS_OPERA_PLATFORM_NOTIFICATION_SERVICE_H_
#define CHILL_BROWSER_NOTIFICATIONS_OPERA_PLATFORM_NOTIFICATION_SERVICE_H_

#include <string>

#include "base/memory/singleton.h"
#include "base/observer_list.h"
#include "content/public/browser/platform_notification_service.h"

namespace opera {

class NotificationBridge;

class OperaPlatformNotificationService
    : public content::PlatformNotificationService {
 public:
  static OperaPlatformNotificationService* GetInstance();

  class Observer {
   public:
    virtual void OnDisplayNotification(
        const GURL& origin,
        const content::PlatformNotificationData& notification_data) = 0;
  };

  void AddObserver(Observer* o) { observer_list_.AddObserver(o); }
  void RemoveObserver(Observer* o) { observer_list_.RemoveObserver(o); }

  blink::mojom::PermissionStatus CheckPermissionOnUIThread(
      content::BrowserContext* browser_context,
      const GURL& origin,
      int render_process_id) override;

  blink::mojom::PermissionStatus CheckPermissionOnIOThread(
      content::ResourceContext* resource_context,
      const GURL& origin,
      int render_process_id) override;

  void DisplayNotification(
      content::BrowserContext* browser_context,
      const std::string& notification_id,
      const GURL& origin,
      const content::PlatformNotificationData& notification_data,
      const content::NotificationResources& notification_resources,
      std::unique_ptr<content::DesktopNotificationDelegate> delegate,
      base::Closure* cancel_callback) override;

  void DisplayPersistentNotification(
      content::BrowserContext* browser_context,
      const std::string& notification_id,
      const GURL& service_worker_origin,
      const GURL& origin,
      const content::PlatformNotificationData& notification_data,
      const content::NotificationResources& notification_resources) override;

  void ClosePersistentNotification(
      content::BrowserContext* browser_context,
      const std::string& notification_id) override;

  bool GetDisplayedPersistentNotifications(
      content::BrowserContext* browser_context,
      std::set<std::string>* displayed_notifications) override;

 private:
  void DisplayNotificationInternal(
      content::BrowserContext* browser_context,
      const GURL& origin,
      const content::PlatformNotificationData& notification_data,
      const content::NotificationResources& notification_resources,
      bool is_persistent,
      const std::string& notification_id,
      content::DesktopNotificationDelegate* delegate);

  NotificationBridge* bridge_;
  base::ObserverList<Observer> observer_list_;

  friend struct base::DefaultSingletonTraits<OperaPlatformNotificationService>;

  OperaPlatformNotificationService();
  ~OperaPlatformNotificationService() override;

  DISALLOW_COPY_AND_ASSIGN(OperaPlatformNotificationService);
};

}  // namespace opera

#endif  // CHILL_BROWSER_NOTIFICATIONS_OPERA_PLATFORM_NOTIFICATION_SERVICE_H_
