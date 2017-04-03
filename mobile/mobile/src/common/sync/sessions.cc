// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sessions.h"

#include <string>
#include <vector>

#include "base/android/jni_string.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "common/sync/session_tab.h"
#include "common/sync/session_window.h"
#include "common/sync/sync.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/sync_sessions/synced_session.h"
#include "components/sync_sessions/open_tabs_ui_delegate.h"
#include "mobile/common/sync/sync_manager.h"

#include "jni/SyncedSession_jni.h"

namespace mobile {

// static
bool Sessions::RegisterJni(JNIEnv* env) {
  return android::RegisterNativesImpl(env) && SessionWindow::RegisterJni(env);
}

// static
base::android::ScopedJavaLocalRef<jobjectArray> Sessions::GetSessions(
    JNIEnv* env) {
  std::vector<const sync_sessions::SyncedSession*> sessions;
  sync_sessions::OpenTabsUIDelegate* open_tabs = GetOpenTabsUIDelegate();
  if (open_tabs)
    open_tabs->GetAllForeignSessions(&sessions);
  base::android::ScopedJavaLocalRef<jobjectArray> ret(
      env,
      env->NewObjectArray(
          sessions.size(), SyncedSession_clazz(env), NULL));
  for (size_t i = 0; i < sessions.size(); i++) {
    const sync_sessions::SyncedSession* session = sessions[i];
    base::android::ScopedJavaLocalRef<jobject> obj =
        mobile::android::Java_SyncedSession_create(
            env,
            base::android::ConvertUTF8ToJavaString(
                env, session->session_tag).obj(),
            base::android::ConvertUTF8ToJavaString(
                env, session->session_name).obj(),
            session->device_type,
            session->modified_time.ToJavaTime());
    env->SetObjectArrayElement(ret.obj(), i, obj.obj());
  }
  return ret;
}

// static
sync_sessions::OpenTabsUIDelegate* Sessions::GetOpenTabsUIDelegate() {
  SyncManager* manager = Sync::GetInstance()->GetSyncManager();
  if (!manager)
    return NULL;
  browser_sync::ProfileSyncService* service =
      ProfileSyncServiceFactory::GetInstance()->GetForProfile(
          manager->GetProfile());
  if (service && service->IsSyncActive())
    return service->GetOpenTabsUIDelegate();
  return NULL;
}

namespace android {

// static
base::android::ScopedJavaLocalRef<jobjectArray> GetSessionWindows(
    JNIEnv* env,
    const base::android::JavaParamRef<jclass>& jcaller,
    const base::android::JavaParamRef<jstring>& tag) {
  return SessionWindow::GetSessionWindows(
      env, base::android::ConvertJavaStringToUTF8(env, tag));
}

}  // namespace android

}  // namespace mobile
