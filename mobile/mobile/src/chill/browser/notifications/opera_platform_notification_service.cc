// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/notifications/opera_platform_notification_service.h"

#include <vector>

#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/scoped_java_ref.h"
#include "base/bind.h"
#include "base/strings/string16.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/permission_manager.h"
#include "content/public/browser/permission_type.h"
#include "content/public/common/notification_resources.h"
#include "content/public/common/platform_notification_data.h"
#include "ui/gfx/android/java_bitmap.h"
#include "url/gurl.h"

#include "chill/app/native_interface.h"
#include "chill/browser/opera_browser_context.h"
#include "chill/browser/opera_resource_context.h"
#include "chill/browser/notifications/notification_bridge.h"

using base::android::AttachCurrentThread;
using base::android::ScopedJavaLocalRef;
using base::android::ToJavaArrayOfStrings;
using base::android::ToJavaIntArray;

namespace opera {

// static
OperaPlatformNotificationService*
OperaPlatformNotificationService::GetInstance() {
  return base::Singleton<OperaPlatformNotificationService>::get();
}

OperaPlatformNotificationService::OperaPlatformNotificationService() {
  bridge_ = NativeInterface::Get()->GetNotificationBridge();
}

OperaPlatformNotificationService::~OperaPlatformNotificationService() {
}

// To avoid code duplication CheckPermissionOnUIThread is made thread safe
// so that it can be called from CheckPermissionOnIOThread. Locking is handled
// in the PermissionManager on the Java side.
blink::mojom::PermissionStatus
OperaPlatformNotificationService::CheckPermissionOnUIThread(
    content::BrowserContext* browser_context,
    const GURL& origin,
    int render_process_id) {
  content::PermissionManager* pm = browser_context->GetPermissionManager();
  return pm->GetPermissionStatus(
      content::PermissionType::NOTIFICATIONS, origin, origin);
}

blink::mojom::PermissionStatus
OperaPlatformNotificationService::CheckPermissionOnIOThread(
    content::ResourceContext* resource_context,
    const GURL& origin,
    int render_process_id) {
  bool private_mode = static_cast<OperaResourceContext*>(resource_context)
      ->IsOffTheRecord();

  OperaBrowserContext* context = private_mode ?
      OperaBrowserContext::GetPrivateBrowserContext() :
      OperaBrowserContext::GetDefaultBrowserContext();

  return CheckPermissionOnUIThread(context, origin, render_process_id);
}

void OperaPlatformNotificationService::DisplayNotification(
    content::BrowserContext* browser_context,
    const std::string& notification_id,
    const GURL& origin,
    const content::PlatformNotificationData& notification_data,
    const content::NotificationResources& notification_resources,
    std::unique_ptr<content::DesktopNotificationDelegate> delegate,
    base::Closure* cancel_callback) {

  DisplayNotificationInternal(
      browser_context,
      origin,
      notification_data,
      notification_resources,
      false,
      0,
      delegate.release());

  *cancel_callback = base::Bind(
      &NotificationBridge::CloseNotification,
      base::Unretained(bridge_),
      notification_id);
}

void OperaPlatformNotificationService::DisplayPersistentNotification(
    content::BrowserContext* browser_context,
    const std::string& notification_id,
    const GURL& service_worker_origin,
    const GURL& origin,
    const content::PlatformNotificationData& notification_data,
    const content::NotificationResources& notification_resources) {

  DisplayNotificationInternal(
      browser_context,
      origin,
      notification_data,
      notification_resources,
      true,
      notification_id,
      nullptr);
}

void OperaPlatformNotificationService::ClosePersistentNotification(
    content::BrowserContext* browser_context,
    const std::string& notification_id) {
  bridge_->ClosePersistentNotification(notification_id);
}

bool OperaPlatformNotificationService::GetDisplayedPersistentNotifications(
    content::BrowserContext* browser_context,
    std::set<std::string>* displayed_notifications) {
  return false;
}

void OperaPlatformNotificationService::DisplayNotificationInternal(
    content::BrowserContext* browser_context,
    const GURL& origin,
    const content::PlatformNotificationData& notification_data,
    const content::NotificationResources& notification_resources,
    bool is_persistent,
    const std::string& notification_id,
    content::DesktopNotificationDelegate* delegate) {

  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jobject> icon;

  if (!notification_resources.notification_icon.drawsNothing())
    icon = gfx::ConvertToJavaBitmap(&notification_resources.notification_icon);

  ScopedJavaLocalRef<jintArray> vibration_pattern =
      ToJavaIntArray(env, notification_data.vibration_pattern);

  std::vector<base::string16> action_titles_vector;

  for (const content::PlatformNotificationAction& action :
      notification_data.actions)
    action_titles_vector.push_back(action.title);

  ScopedJavaLocalRef<jobjectArray> action_titles =
      ToJavaArrayOfStrings(env, action_titles_vector);

  FOR_EACH_OBSERVER(Observer, observer_list_,
                    OnDisplayNotification(origin, notification_data));

  bridge_->DisplayNotification(
      origin.GetOrigin().spec(),
      browser_context->IsOffTheRecord(),
      notification_data.tag,
      notification_data.title,
      notification_data.body,
      icon.obj(),
      notification_data.timestamp.ToJavaTime(),
      notification_data.silent,
      vibration_pattern.obj(),
      action_titles.obj(),
      is_persistent,
      notification_id,
      delegate);
}

}  // namespace opera
