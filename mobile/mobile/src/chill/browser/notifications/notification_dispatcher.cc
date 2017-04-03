// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/notifications/notification_dispatcher.h"

#include "base/android/jni_android.h"
#include "base/bind.h"
#include "content/public/browser/notification_event_dispatcher.h"
#include "url/gurl.h"

#include "chill/app/native_interface.h"
#include "chill/browser/notifications/notification_bridge.h"
#include "chill/browser/opera_browser_context.h"

using base::android::AttachCurrentThread;
using base::android::ScopedJavaGlobalRef;

namespace opera {

static void Callback(ScopedJavaGlobalRef<jobject> callback,
    content::PersistentNotificationStatus) {
  NotificationBridge* bridge = NativeInterface::Get()->GetNotificationBridge();
  bridge->DispatchDone(callback.obj());
}

// static
void NotificationDispatcher::OnClick(
    bool private_mode,
    const std::string& notification_id,
    const std::string& origin,
    int action_index,
    jobject callback) {

  JNIEnv* env = AttachCurrentThread();

  OperaBrowserContext* context = private_mode ?
      OperaBrowserContext::GetPrivateBrowserContext() :
      OperaBrowserContext::GetDefaultBrowserContext();

  content::NotificationEventDispatcher::GetInstance()->
      DispatchNotificationClickEvent(
          context,
          notification_id,
          GURL(origin),
          action_index,
          base::Bind(&Callback, ScopedJavaGlobalRef<jobject>(env, callback)));
}

// static
void NotificationDispatcher::OnClose(
    bool private_mode,
    const std::string& notification_id,
    const std::string& origin,
    bool by_user,
    jobject callback) {

  JNIEnv* env = AttachCurrentThread();

  OperaBrowserContext* context = private_mode ?
      OperaBrowserContext::GetPrivateBrowserContext() :
      OperaBrowserContext::GetDefaultBrowserContext();

  content::NotificationEventDispatcher::GetInstance()->
      DispatchNotificationCloseEvent(
          context,
          notification_id,
          GURL(origin),
          by_user,
          base::Bind(&Callback, ScopedJavaGlobalRef<jobject>(env, callback)));
}

}  // namespace opera
