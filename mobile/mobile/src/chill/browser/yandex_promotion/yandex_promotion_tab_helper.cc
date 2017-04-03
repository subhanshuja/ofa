// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#include "chill/browser/yandex_promotion/yandex_promotion_tab_helper.h"

#include <utility>

#include "content/public/browser/render_frame_host.h"

#include "base/android/jni_string.h"
#include "content/public/browser/web_contents.h"
#include "jni/YandexPromotionTabHelper_jni.h"

#include "chill/common/yandex_promotion_messages.h"

namespace opera {

YandexPromotionTabHelper::YandexPromotionTabHelper(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& java_ref,
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents), java_ref_(env, java_ref) {}

void YandexPromotionTabHelper::WebContentsDestroyed() {
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_YandexPromotionTabHelper_destroy(env, java_ref_.obj());

  delete this;
}

bool YandexPromotionTabHelper::OnMessageReceived(
    const IPC::Message& message,
    content::RenderFrameHost* render_frame_host) {
  if (web_contents()->GetMainFrame() != render_frame_host)
    return false;

  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(YandexPromotionTabHelper, message)
    IPC_MESSAGE_HANDLER(YandexMsg_CanAskToSetAsDefault, OnCanAskToSetAsDefault)
    IPC_MESSAGE_HANDLER(YandexMsg_AskToSetAsDefault, OnAskToSetAsDefault)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void YandexPromotionTabHelper::AskToSetAsDefaultReply(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& java_ref,
    jint request_id) {
  content::RenderFrameHost* render_frame_host = web_contents()->GetMainFrame();
  render_frame_host->Send(new YandexMsg_AskToSetAsDefaultReply(
      render_frame_host->GetRoutingID(), static_cast<uint32_t>(request_id)));
}

void YandexPromotionTabHelper::OnCanAskToSetAsDefault(
    uint32_t request_id,
    const std::string& engine) {
  JNIEnv* env = base::android::AttachCurrentThread();
  bool response = Java_YandexPromotionTabHelper_canAskToSetAsDefault(
      env, java_ref_.obj(),
      base::android::ConvertUTF8ToJavaString(env, engine).obj());

  content::RenderFrameHost* render_frame_host = web_contents()->GetMainFrame();
  render_frame_host->Send(new YandexMsg_CanAskToSetAsDefaultReply(
      render_frame_host->GetRoutingID(), request_id, response));
}

void YandexPromotionTabHelper::OnAskToSetAsDefault(uint32_t request_id,
                                                   const std::string& engine) {
  JNIEnv* env = base::android::AttachCurrentThread();

  Java_YandexPromotionTabHelper_askToSetAsDefault(
      env, java_ref_.obj(),
      base::android::ConvertUTF8ToJavaString(env, engine).obj(),
      static_cast<jint>(request_id));
}

bool YandexPromotionTabHelper::Register(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

jlong Init(JNIEnv* env,
           const base::android::JavaParamRef<jobject>& java_ref,
           const base::android::JavaParamRef<jobject>& j_web_contents) {
  return reinterpret_cast<intptr_t>(new YandexPromotionTabHelper(
      env, java_ref,
      content::WebContents::FromJavaWebContents(j_web_contents)));
}

}  // namespace opera
