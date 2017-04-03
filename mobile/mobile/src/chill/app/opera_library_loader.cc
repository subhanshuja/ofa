// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/android/shell_library_loader.cc
// @final-synchronized

#include "base/android/jni_android.h"
#include "base/bind.h"
#include "content/public/app/content_jni_onload.h"
#include "content/public/app/content_main.h"
#include "content/public/browser/android/compositor.h"

#include "chill/app/opera_main_delegate.h"
#include "chill/browser/opera_jni_registrar.h"
#include "common/sync/sync.h"
#ifdef ENABLE_BROWSER_PROVIDER
#include "common/browser_provider/browser_provider.h"
#endif

namespace {

bool RegisterJNI(JNIEnv* env) {
  if (!mobile::Sync::RegisterJni(env))
    return false;
  if (!opera::RegisterJni(env))
    return false;
#ifdef ENABLE_BROWSER_PROVIDER
  if (!provider::android::RegisterJni(env))
    return false;
#endif
  return true;
}

bool Init() {
  content::Compositor::Initialize();
  content::SetContentMainDelegate(new opera::OperaMainDelegate());
  return true;
}

}  // namespace

// This is called by the VM when the shared library is first loaded.
JNI_EXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  std::vector<base::android::RegisterCallback> register_callbacks;
  register_callbacks.push_back(base::Bind(&RegisterJNI));
  std::vector<base::android::InitCallback> init_callbacks;
  init_callbacks.push_back(base::Bind(&Init));
  if (!content::android::OnJNIOnLoadRegisterJNI(vm, register_callbacks) ||
      !content::android::OnJNIOnLoadInit(init_callbacks)) {
    return -1;
  }

  return JNI_VERSION_1_4;
}
