// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2017 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#include "chill/browser/login/oauth2_account.h"

#include <string>

#include "base/android/callback_android.h"
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "chill/browser/login/oauth2_account_service.h"
#include "chill/browser/login/oauth2_account_service_factory.h"
#include "chill/browser/opera_browser_context.h"
#include "chrome/browser/profiles/profile.h"
#include "jni/OAuth2Account_jni.h"
#include "jni/LoginResult_jni.h"
#include "mobile/common/sync/sync_browser_process.h"
#include "mobile/common/sync/sync_profile.h"
#include "mobile/common/sync/sync_profile_manager.h"

namespace {

void RunCallback(
    base::android::ScopedJavaGlobalRef<jobject> j_callback,
    const opera::OAuth2AccountService::SessionStartResult& start_result) {
  DCHECK((start_result.username && start_result.error == opera::ERR_NONE) ||
         start_result.error != opera::ERR_NONE);
  JNIEnv* env = base::android::AttachCurrentThread();

  base::android::RunCallbackAndroid(
      j_callback,
      start_result.error == opera::ERR_NONE
          ? opera::Java_LoginResult_forUser(
                env, base::android::ConvertUTF8ToJavaString(
                         env, start_result.username.value()))
          : opera::Java_LoginResult_forError(env, start_result.error));
}

}  // namespace

namespace opera {

OAuth2Account::OAuth2Account(
    const base::android::JavaParamRef<jobject>& j_account,
    OAuth2AccountService* account_service)
    : account_service_(account_service), java_account_(j_account) {
  account_service_->AddObserver(this);
}

OAuth2Account::~OAuth2Account() {
  account_service_->RemoveObserver(this);
}

void OAuth2Account::Login(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& java_ref,
    const base::android::JavaParamRef<jstring>& j_username,
    const base::android::JavaParamRef<jstring>& j_password,
    const base::android::JavaParamRef<jobject>& j_callback) {
  account_service_->StartSessionWithCredentials(
      base::android::ConvertJavaStringToUTF8(j_username),
      base::android::ConvertJavaStringToUTF8(j_password),
      base::Bind(&RunCallback,
                 base::android::ScopedJavaGlobalRef<jobject>(j_callback)));
}

void OAuth2Account::LoginWithToken(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& java_ref,
    const base::android::JavaParamRef<jstring>& j_username,
    const base::android::JavaParamRef<jstring>& j_token,
    const base::android::JavaParamRef<jobject>& j_callback) {
  account_service_->StartSessionWithAuthToken(
      base::android::ConvertJavaStringToUTF8(j_username),
      base::android::ConvertJavaStringToUTF8(j_token),
      base::Bind(&RunCallback,
                 base::android::ScopedJavaGlobalRef<jobject>(j_callback)));
}

void OAuth2Account::Logout(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& java_ref) {
  account_service_->EndSession();
}

bool OAuth2Account::IsLoggedIn(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& java_ref) {
  return account_service_->IsLoggedIn();
}

bool OAuth2Account::Register(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

void OAuth2Account::OnServiceDestroyed() {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobject> obj(java_account_);
  Java_OAuth2Account_onDestroy(env, obj);

  delete this;
}

void OAuth2Account::OnLoggedOut() {
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaLocalRef<jobject> obj(java_account_);
  Java_OAuth2Account_onLoggedOut(env, obj);
}

jlong Init(JNIEnv* env, const base::android::JavaParamRef<jobject>& jcaller) {
  SyncProfileManager* profile_manager =
      static_cast<SyncProfileManager*>(g_browser_process->profile_manager());
  Profile* profile = static_cast<Profile*>(profile_manager->profile());
  DCHECK(profile);

  OAuth2AccountService* account_service =
      OAuth2AccountServiceFactory::GetInstance()->GetForProfile(profile);

  return reinterpret_cast<intptr_t>(
      new OAuth2Account(jcaller, account_service));
}

}  // namespace opera
