// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/favorites/speed_dial_data_fetcher.h"

#include "base/android/jni_android.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "ui/gfx/android/java_bitmap.h"

#include "chill/common/opera_render_view_messages.h"
#include "chill/common/webapps/web_application_info.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(opera::SpeedDialDataFetcher);

namespace opera {

SpeedDialDataFetcher::SpeedDialDataFetcher(content::WebContents* web_contents,
                                           int ideal_icon_size_in_px)
    : WebContentsObserver(web_contents),
      favicon_selector_(web_contents),
      did_get_web_app_info_(false),
      is_icon_saved_(false),
      icon_timeout_timer_(false, false),
      ideal_icon_size_in_px_(ideal_icon_size_in_px),
      weak_ptr_factory_(this) {
  // Hold on to the web contents to keep alive.
  web_contents->SetUserData(UserDataKey(), this);
}

SpeedDialDataFetcher::SpeedDialDataFetcher(jobject j_web_contents,
                                           int ideal_icon_size_in_px)
    : SpeedDialDataFetcher(content::WebContents::FromJavaWebContents(
                               base::android::ScopedJavaLocalRef<jobject>(
                                   base::android::AttachCurrentThread(),
                                   j_web_contents)),
                           ideal_icon_size_in_px) {}

void SpeedDialDataFetcher::Initialize() {
  // Send a message to the renderer to retrieve information about the page.
  Send(new OpViewMsg_GetWebApplicationInfo(routing_id()));
}

void SpeedDialDataFetcher::TearDown() {
  web_contents()->RemoveUserData(UserDataKey());
}

bool SpeedDialDataFetcher::IsInProgress(jobject j_web_contents) {
  // Check if there is a SpeedDialDataFetcher instance alive.
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(
          base::android::ScopedJavaLocalRef<jobject>(
              base::android::AttachCurrentThread(), j_web_contents));
  return web_contents->GetUserData(UserDataKey()) != nullptr;
}

void SpeedDialDataFetcher::OnDidGetWebApplicationInfo(
    const WebApplicationInfo& web_app_info) {
  // Make sure we receive webapp info only once.
  if (did_get_web_app_info_)
    return;
  did_get_web_app_info_ = true;

  // Kick off a timeout for downloading the icon.  If an icon isn't set within
  // the timeout, fall back to using a dynamically-generated launcher icon.
  icon_timeout_timer_.Start(
      FROM_HERE, base::TimeDelta::FromSeconds(10),
      base::Bind(&SpeedDialDataFetcher::OnIconFetchTimeoutExpired,
                 weak_ptr_factory_.GetWeakPtr()));

  favicon_selector_.FindBestMatchingIconPX(
      web_app_info.icons, ideal_icon_size_in_px_,
      base::Bind(&SpeedDialDataFetcher::OnFaviconFetched,
                 weak_ptr_factory_.GetWeakPtr()));
}

void SpeedDialDataFetcher::OnFaviconFetched(const SkBitmap& bitmap) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (is_icon_saved_)
    return;

  is_icon_saved_ = true;

  base::android::ScopedJavaLocalRef<jobject> java_bitmap;
  if (bitmap.getSize())
    java_bitmap = gfx::ConvertToJavaBitmap(&bitmap);

  FinalizeAndSetIcon(java_bitmap.obj());

  TearDown();
}

void SpeedDialDataFetcher::OnIconFetchTimeoutExpired() {
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&SpeedDialDataFetcher::OnFaviconFetched,
                 weak_ptr_factory_.GetWeakPtr(), SkBitmap()));
}

bool SpeedDialDataFetcher::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;

  IPC_BEGIN_MESSAGE_MAP(SpeedDialDataFetcher, message)
  IPC_MESSAGE_HANDLER(OperaViewHostMsg_DidGetWebApplicationInfo,
                      OnDidGetWebApplicationInfo)
  IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

    return handled;
}

}  // namespace opera
