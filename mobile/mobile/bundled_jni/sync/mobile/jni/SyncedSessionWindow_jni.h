// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is autogenerated by
//     base/android/jni_generator/jni_generator.py
// For
//     com/opera/android/sync/SyncedSessionWindow

#ifndef com_opera_android_sync_SyncedSessionWindow_JNI
#define com_opera_android_sync_SyncedSessionWindow_JNI

#include <jni.h>

#include "../../../../../../../../../../../../../../../../base/android/jni_generator/jni_generator_helper.h"

#include "base/android/jni_int_wrapper.h"

// Step 1: forward declarations.
namespace {
const char kSyncedSessionWindowClassPath[] =
    "com/opera/android/sync/SyncedSessionWindow";
// Leaking this jclass as we cannot use LazyInstance from some threads.
base::subtle::AtomicWord g_SyncedSessionWindow_clazz __attribute__((unused)) =
    0;
#define SyncedSessionWindow_clazz(env) base::android::LazyGetClass(env, kSyncedSessionWindowClassPath, &g_SyncedSessionWindow_clazz)

}  // namespace

namespace mobile {
namespace android {

// Step 2: method stubs.

static base::android::ScopedJavaLocalRef<jobject> GetWindowTab(JNIEnv* env,
    const base::android::JavaParamRef<jclass>& jcaller,
    const base::android::JavaParamRef<jstring>& tag,
    jlong id,
    jint index);

JNI_GENERATOR_EXPORT jobject
    Java_com_opera_android_sync_SyncedSessionWindow_nativeGetWindowTab(JNIEnv*
    env, jclass jcaller,
    jstring tag,
    jlong id,
    jint index) {
  return GetWindowTab(env, base::android::JavaParamRef<jclass>(env, jcaller),
      base::android::JavaParamRef<jstring>(env, tag), id, index).Release();
}

static base::android::ScopedJavaLocalRef<jobjectArray> GetWindowTabs(JNIEnv*
    env, const base::android::JavaParamRef<jclass>& jcaller,
    const base::android::JavaParamRef<jstring>& tag,
    jlong id);

JNI_GENERATOR_EXPORT jobjectArray
    Java_com_opera_android_sync_SyncedSessionWindow_nativeGetWindowTabs(JNIEnv*
    env, jclass jcaller,
    jstring tag,
    jlong id) {
  return GetWindowTabs(env, base::android::JavaParamRef<jclass>(env, jcaller),
      base::android::JavaParamRef<jstring>(env, tag), id).Release();
}

static base::subtle::AtomicWord g_SyncedSessionWindow_create = 0;
static base::android::ScopedJavaLocalRef<jobject>
    Java_SyncedSessionWindow_create(JNIEnv* env, const
    base::android::JavaRefOrBare<jstring>& sessionTag,
    jlong id,
    JniIntWrapper selectedTabIndex,
    JniIntWrapper type,
    jlong modifiedTime) {
  CHECK_CLAZZ(env, SyncedSessionWindow_clazz(env),
      SyncedSessionWindow_clazz(env), NULL);
  jmethodID method_id =
      base::android::MethodID::LazyGet<
      base::android::MethodID::TYPE_STATIC>(
      env, SyncedSessionWindow_clazz(env),
      "create",

"("
"Ljava/lang/String;"
"J"
"I"
"I"
"J"
")"
"Lcom/opera/android/sync/SyncedSessionWindow;",
      &g_SyncedSessionWindow_create);

  jobject ret =
      env->CallStaticObjectMethod(SyncedSessionWindow_clazz(env),
          method_id, sessionTag.obj(), id, as_jint(selectedTabIndex),
              as_jint(type), modifiedTime);
  jni_generator::CheckException(env);
  return base::android::ScopedJavaLocalRef<jobject>(env, ret);
}

// Step 3: RegisterNatives.

static const JNINativeMethod kMethodsSyncedSessionWindow[] = {
    { "nativeGetWindowTab",
"("
"Ljava/lang/String;"
"J"
"I"
")"
"Lcom/opera/android/sync/SyncedSessionTab;",
    reinterpret_cast<void*>(Java_com_opera_android_sync_SyncedSessionWindow_nativeGetWindowTab)
    },
    { "nativeGetWindowTabs",
"("
"Ljava/lang/String;"
"J"
")"
"[Lcom/opera/android/sync/SyncedSessionTab;",
    reinterpret_cast<void*>(Java_com_opera_android_sync_SyncedSessionWindow_nativeGetWindowTabs)
    },
};

static bool RegisterNativesImpl(JNIEnv* env) {
  if (base::android::IsManualJniRegistrationDisabled()) return true;

  const int kMethodsSyncedSessionWindowSize =
      arraysize(kMethodsSyncedSessionWindow);

  if (env->RegisterNatives(SyncedSessionWindow_clazz(env),
                           kMethodsSyncedSessionWindow,
                           kMethodsSyncedSessionWindowSize) < 0) {
    jni_generator::HandleRegistrationError(
        env, SyncedSessionWindow_clazz(env), __FILE__);
    return false;
  }

  return true;
}

}  // namespace android
}  // namespace mobile

#endif  // com_opera_android_sync_SyncedSessionWindow_JNI