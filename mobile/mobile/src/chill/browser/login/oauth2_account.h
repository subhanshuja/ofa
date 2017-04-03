// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2017 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#ifndef CHILL_BROWSER_LOGIN_OAUTH2_ACCOUNT_H_
#define CHILL_BROWSER_LOGIN_OAUTH2_ACCOUNT_H_

#include <jni.h>
#include <string>

#include "base/android/scoped_java_ref.h"
#include "chill/browser/login/oauth2_account_service_observer.h"

namespace opera {
class OAuth2AccountService;

// Bridge between Java and C++ classes
// Breaks the lifetime dependency chain between Java objects and that of
// instances which depend on Profile
class OAuth2Account : public OAuth2AccountServiceObserver {
 public:
  explicit OAuth2Account(const base::android::JavaParamRef<jobject>& j_account,
                         OAuth2AccountService* account_service);

  ~OAuth2Account() override;

  void Login(JNIEnv* env,
             const base::android::JavaParamRef<jobject>& java_ref,
             const base::android::JavaParamRef<jstring>& username,
             const base::android::JavaParamRef<jstring>& password,
             const base::android::JavaParamRef<jobject>& callback);

  void LoginWithToken(JNIEnv* env,
                      const base::android::JavaParamRef<jobject>& java_ref,
                      const base::android::JavaParamRef<jstring>& username,
                      const base::android::JavaParamRef<jstring>& token,
                      const base::android::JavaParamRef<jobject>& callback);

  void Logout(JNIEnv* env,
              const base::android::JavaParamRef<jobject>& java_ref);

  bool IsLoggedIn(JNIEnv* env,
                  const base::android::JavaParamRef<jobject>& java_ref);

  static bool Register(JNIEnv* env);

  // OAuth2AccountServiceObserver implementation
  void OnServiceDestroyed() override;
  void OnLoggedOut() override;

 private:
  // Lifetime governed by SyncProfile
  OAuth2AccountService* account_service_;
  base::android::ScopedJavaGlobalRef<jobject> java_account_;

  DISALLOW_COPY_AND_ASSIGN(OAuth2Account);
};

}  // namespace opera

#endif  // CHILL_BROWSER_LOGIN_OAUTH2_ACCOUNT_H_
