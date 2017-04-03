// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/session_tab.h"

#include <string>

#include "base/android/jni_string.h"
#include "common/sync/sessions.h"
#include "components/sync_sessions/open_tabs_ui_delegate.h"

#include "jni/SyncedSessionTab_jni.h"

namespace mobile {

namespace {

base::android::ScopedJavaLocalRef<jobject> CreateSessionTab(
    JNIEnv* env,
    const sessions::SessionTab* tab) {
  sessions::SerializedNavigationEntry entry;
  if (tab->current_navigation_index >= 0
      && tab->current_navigation_index <
        static_cast<int>(tab->navigations.size())) {
    entry = tab->navigations[tab->current_navigation_index];
  } else if (!tab->navigations.empty()) {
    entry = tab->navigations[tab->navigations.size() - 1];
  }

  return mobile::android::Java_SyncedSessionTab_create(
      env,
      tab->window_id.id(),
      tab->tab_id.id(),
      base::android::ConvertUTF8ToJavaString(env, tab->extension_app_id).obj(),
      tab->timestamp.ToJavaTime(),
      base::android::ConvertUTF8ToJavaString(
          env, entry.virtual_url().spec()).obj(),
      base::android::ConvertUTF8ToJavaString(
          env, entry.original_request_url().spec()).obj(),
      base::android::ConvertUTF16ToJavaString(env, entry.title()).obj());
}

}  // namespace

// static
base::android::ScopedJavaLocalRef<jobjectArray> SessionTab::GetSessionTabs(
    JNIEnv* env,
    const sessions::SessionWindow* window) {
  if (!window)
    return base::android::ScopedJavaLocalRef<jobjectArray>();

  base::android::ScopedJavaLocalRef<jobjectArray> ret(
      env,
      env->NewObjectArray(
          window->tabs.size(), SyncedSessionTab_clazz(env), NULL));
  for (size_t i = 0; i < window->tabs.size(); i++) {
    sessions::SessionTab* tab = window->tabs[i].get();
    env->SetObjectArrayElement(ret.obj(), i, CreateSessionTab(env, tab).obj());
  }
  return ret;
}

// static
base::android::ScopedJavaLocalRef<jobject> SessionTab::GetSessionTab(
    JNIEnv* env,
    const sessions::SessionWindow* window,
    int index) {
  if (!window || index < 0 || index >= static_cast<int>(window->tabs.size()))
    return base::android::ScopedJavaLocalRef<jobject>();

  return CreateSessionTab(env, window->tabs[index].get());
}

}  // namespace mobile
