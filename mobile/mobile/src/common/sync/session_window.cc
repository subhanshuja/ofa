// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/session_window.h"

#include <vector>

#include "base/android/jni_string.h"
#include "common/sync/sessions.h"
#include "common/sync/session_tab.h"
#include "components/sync_sessions/open_tabs_ui_delegate.h"

#include "jni/SyncedSessionWindow_jni.h"

namespace mobile {

namespace {

const sessions::SessionWindow* GetSessionWindow(
    JNIEnv* env,
    const base::android::JavaRef<jstring>& tag,
    jlong id) {
  sync_sessions::OpenTabsUIDelegate* open_tabs =
      Sessions::GetOpenTabsUIDelegate();
  std::vector<const sessions::SessionWindow*> windows;
  std::string session_tag(
      base::android::ConvertJavaStringToUTF8(env, tag.obj()));
  if (open_tabs)
    open_tabs->GetForeignSession(session_tag, &windows);
  for (auto i(windows.begin()); i != windows.end(); ++i) {
    if ((*i)->window_id.id() == id) {
      return *i;
    }
  }
  return NULL;
}

}  // namespace

// static
bool SessionWindow::RegisterJni(JNIEnv* env) {
  return android::RegisterNativesImpl(env);
}

// static
base::android::ScopedJavaLocalRef<jobjectArray>
SessionWindow::GetSessionWindows(JNIEnv* env, const std::string& session_tag) {
  std::vector<const sessions::SessionWindow*> windows;
  sync_sessions::OpenTabsUIDelegate* open_tabs =
      Sessions::GetOpenTabsUIDelegate();
  if (open_tabs)
    open_tabs->GetForeignSession(session_tag, &windows);
  base::android::ScopedJavaLocalRef<jstring> tag(
      base::android::ConvertUTF8ToJavaString(env, session_tag));
  base::android::ScopedJavaLocalRef<jobjectArray> ret(
      env,
      env->NewObjectArray(
          windows.size(), SyncedSessionWindow_clazz(env), NULL));
  for (size_t i = 0; i < windows.size(); i++) {
    const sessions::SessionWindow* window = windows[i];
    base::android::ScopedJavaLocalRef<jobject> obj =
        mobile::android::Java_SyncedSessionWindow_create(
            env,
            tag.obj(),
            window->window_id.id(),
            window->selected_tab_index,
            window->type,
            window->timestamp.ToJavaTime());
    env->SetObjectArrayElement(ret.obj(), i, obj.obj());
  }
  return ret;
}

namespace android {

// static
base::android::ScopedJavaLocalRef<jobject> GetWindowTab(
    JNIEnv* env,
    const base::android::JavaParamRef<jclass>& caller,
    const base::android::JavaParamRef<jstring>& tag,
    jlong id,
    jint index) {
  return SessionTab::GetSessionTab(env, GetSessionWindow(env, tag, id), index);
}

// static
base::android::ScopedJavaLocalRef<jobjectArray> GetWindowTabs(
    JNIEnv* env,
    const base::android::JavaParamRef<jclass>& caller,
    const base::android::JavaParamRef<jstring>& tag,
    jlong id) {
  return SessionTab::GetSessionTabs(env, GetSessionWindow(env, tag, id));
}

}  // namespace android

}  // namespace mobile
