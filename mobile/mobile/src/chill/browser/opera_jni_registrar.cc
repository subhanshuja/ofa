// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/browser/android/chrome_jni_registrar.cc
// @final-synchronized

#include "chill/browser/opera_jni_registrar.h"

#include "base/android/jni_android.h"
#include "base/android/jni_registrar.h"
#include "components/gcm_driver/gcm_driver_android.h"
#include "components/web_contents_delegate_android/component_jni_registrar.h"

#include "chill/browser/articles/article.h"
#include "chill/browser/articles/article_overlay.h"
#include "chill/browser/login/oauth2_account.h"
#include "chill/browser/service_tab_launcher.h"
#include "chill/browser/ui/opera_bluetooth_chooser_android.h"
#include "chill/browser/yandex_promotion/yandex_promotion_tab_helper.h"

namespace opera {

static base::android::RegistrationMethod kOperaRegisteredMethods[] = {
    {"ArticleOverlay", ArticleOverlay::Register},
    {"BluetoothChooserAndroid", BluetoothChooserAndroid::Register},
    {"GCMDriver", gcm::GCMDriverAndroid::RegisterJni},
    {"OAuth2Account", OAuth2Account::Register},
    {"ServiceTabLauncher", ServiceTabLauncher::Register},
    {"WebContentsDelegateAndroid",
     web_contents_delegate_android::RegisterWebContentsDelegateAndroidJni},
    {"YandexPromotionTabHelper", YandexPromotionTabHelper::Register},
};

bool RegisterJni(JNIEnv* env) {
  return RegisterNativeMethods(env, kOperaRegisteredMethods,
                               arraysize(kOperaRegisteredMethods));
}

}  // namespace opera
