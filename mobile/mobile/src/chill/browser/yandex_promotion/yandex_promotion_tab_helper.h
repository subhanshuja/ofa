// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#ifndef CHILL_BROWSER_YANDEX_PROMOTION_YANDEX_PROMOTION_TAB_HELPER_H_
#define CHILL_BROWSER_YANDEX_PROMOTION_YANDEX_PROMOTION_TAB_HELPER_H_

#include <jni.h>
#include <string>
#include <unordered_map>

#include "base/android/scoped_java_ref.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class WebContents;
}  // content

namespace opera {

class YandexPromotionTabHelper : public content::WebContentsObserver {
 public:
  YandexPromotionTabHelper(JNIEnv* env,
                           const base::android::JavaParamRef<jobject>& java_ref,
                           content::WebContents* web_contents);

  void AskToSetAsDefaultReply(
      JNIEnv* env,
      const base::android::JavaParamRef<jobject>& java_ref,
      jint request_id);

  static bool Register(JNIEnv* env);

 private:
  // WebContentsObserver implementation
  bool OnMessageReceived(const IPC::Message& message,
                         content::RenderFrameHost* render_frame_host) override;

  void WebContentsDestroyed() override;

  // IPC message handlers
  void OnCanAskToSetAsDefault(uint32_t request_id, const std::string& engine);
  void OnAskToSetAsDefault(uint32_t request_id, const std::string& engine);

  base::android::ScopedJavaGlobalRef<jobject> java_ref_;

  DISALLOW_COPY_AND_ASSIGN(YandexPromotionTabHelper);
};

}  // namespace opera

#endif  // CHILL_BROWSER_YANDEX_PROMOTION_YANDEX_PROMOTION_TAB_HELPER_H_
