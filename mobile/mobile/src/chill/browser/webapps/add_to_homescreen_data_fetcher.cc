// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/browser/android/webapps/add_to_homescreen_data_fetcher.cc
// @final-synchronized

#include "chill/browser/webapps/add_to_homescreen_data_fetcher.h"

#include "base/android/jni_android.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/origin_util.h"
#include "ui/gfx/android/java_bitmap.h"
#include "third_party/WebKit/public/platform/modules/screen_orientation/WebScreenOrientationLockType.h"
#include "third_party/WebKit/public/platform/WebDisplayMode.h"

#include "chill/browser/webapps/manifest_icon_downloader.h"
#include "chill/browser/webapps/manifest_icon_selector.h"
#include "chill/common/opera_render_view_messages.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(opera::AddToHomescreenDataFetcher);

namespace opera {

AddToHomescreenDataFetcher::AddToHomescreenDataFetcher(
    content::WebContents* web_contents,
    int ideal_icon_size_in_dp,
    int minimum_icon_size_in_dp)
    : WebContentsObserver(web_contents),
      favicon_selector_(web_contents),
      did_get_web_app_info_(false),
      is_icon_saved_(false),
      icon_timeout_timer_(false, false),
      ideal_icon_size_in_dp_(ideal_icon_size_in_dp),
      minimum_icon_size_in_dp_(minimum_icon_size_in_dp),
      weak_ptr_factory_(this) {
  // Hold on to the web contents to keep alive.
  web_contents->SetUserData(UserDataKey(), this);
}

AddToHomescreenDataFetcher::AddToHomescreenDataFetcher(
    jobject j_web_contents,
    int ideal_icon_size_in_dp,
    int minimum_icon_size_in_dp)
    : AddToHomescreenDataFetcher(content::WebContents::FromJavaWebContents(
                                     base::android::ScopedJavaLocalRef<jobject>(
                                         base::android::AttachCurrentThread(),
                                         j_web_contents)),
                                 ideal_icon_size_in_dp,
                                 minimum_icon_size_in_dp) {}

void AddToHomescreenDataFetcher::Initialize() {
  // Send a message to the renderer to retrieve information about the page.
  Send(new OpViewMsg_GetWebApplicationInfo(routing_id()));
}

void AddToHomescreenDataFetcher::TearDown() {
  web_contents()->RemoveUserData(UserDataKey());
}

bool AddToHomescreenDataFetcher::IsInProgress(jobject j_web_contents) {
  // Check if there is a AddToHomescreenDataFetcher instance alive.
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(
          base::android::ScopedJavaLocalRef<jobject>(
              base::android::AttachCurrentThread(), j_web_contents));
  return web_contents->GetUserData(UserDataKey()) != nullptr;
}

void AddToHomescreenDataFetcher::OnDidGetWebApplicationInfo(
    const WebApplicationInfo& received_web_app_info) {
  // Make sure we receive webapp info only once.
  if (did_get_web_app_info_)
    return;
  did_get_web_app_info_ = true;

  // Sanitize received_web_app_info.
  web_app_info_ = received_web_app_info;
  web_app_info_.title =
      web_app_info_.title.substr(0, kMaxMetaTagAttributeLength);
  web_app_info_.description =
      web_app_info_.description.substr(0, kMaxMetaTagAttributeLength);

  title_ = web_app_info_.title.empty() ? web_contents()->GetTitle()
                                       : web_app_info_.title;
  short_name_ = title_;
  name_ = title_;

  url_ = web_app_info_.app_url;
  if (!url_.is_valid()) {
    url_ = web_contents()->GetURL();
  }

  web_contents()->GetManifest(
      base::Bind(&AddToHomescreenDataFetcher::OnDidGetManifest,
                 weak_ptr_factory_.GetWeakPtr()));
}

void AddToHomescreenDataFetcher::OnDidGetManifest(
    const GURL& manifest_url,
    const content::Manifest& manifest) {
  // Update url based on the manifest value, if any.
  if (manifest.start_url.is_valid())
    url_ = manifest.start_url;

  bool is_secure = content::IsOriginSecure(url_);

  // Support for pages marked as mobile-capable via apple specific meta tag.
  bool is_mobile_capable =
      web_app_info_.mobile_capable == WebApplicationInfo::MOBILE_CAPABLE ||
      web_app_info_.mobile_capable == WebApplicationInfo::MOBILE_CAPABLE_APPLE;

  // Get the display mode from manifest, or use standalone if mobile-capable.
  blink::WebDisplayMode display = manifest.display;
  if (is_mobile_capable && display == blink::WebDisplayModeUndefined)
    display = blink::WebDisplayModeStandalone;

  // Any secure url with a valid display mode is installable.
  bool is_webapp_capable = is_secure &&
                           display != blink::WebDisplayModeUndefined &&
                           display != blink::WebDisplayModeBrowser;

  // Update name and title based on the manifest value, if any.
  if (!manifest.short_name.is_null())
    short_name_ = manifest.short_name.string();
  if (!manifest.name.is_null())
    name_ = manifest.name.string();
  if (manifest.short_name.is_null() != manifest.name.is_null()) {
    if (manifest.short_name.is_null())
      short_name_ = name_;
    else
      name_ = short_name_;
  }
  title_ = short_name_;

  OnInitialized(title_, short_name_, name_, url_, display, manifest.orientation,
                is_webapp_capable, manifest.background_color,
                manifest.theme_color);

  GURL icon_src = ManifestIconSelector::FindBestMatchingIcon(
      manifest.icons,
      ideal_icon_size_in_dp_,
      minimum_icon_size_in_dp_);

  if (!ManifestIconDownloader::Download(
        web_contents(),
        icon_src,
        ideal_icon_size_in_dp_,
        minimum_icon_size_in_dp_,
        base::Bind(&AddToHomescreenDataFetcher::OnManifestIconFetched,
                   weak_ptr_factory_.GetWeakPtr()))) {
    FetchFavicon();
  }

  // Kick off a timeout for downloading the icon.  If an icon isn't set within
  // the timeout, fall back to using a dynamically-generated launcher icon.
  icon_timeout_timer_.Start(
      FROM_HERE, base::TimeDelta::FromSeconds(10),
      base::Bind(&AddToHomescreenDataFetcher::OnIconFetchTimeoutExpired,
                 weak_ptr_factory_.GetWeakPtr()));
}

void AddToHomescreenDataFetcher::OnManifestIconFetched(const SkBitmap& bitmap) {
  if (bitmap.drawsNothing()) {
    FetchFavicon();
    return;
  }

  is_icon_saved_ = true;

  base::android::ScopedJavaLocalRef<jobject> java_bitmap;
  if (bitmap.getSize())
    java_bitmap = gfx::ConvertToJavaBitmap(&bitmap);
  SetIcon(java_bitmap.obj());
}

void AddToHomescreenDataFetcher::FetchFavicon() {
  favicon_selector_.FindBestMatchingIconDP(
      web_app_info_.icons, ideal_icon_size_in_dp_,
      base::Bind(&AddToHomescreenDataFetcher::OnFaviconFetched,
                 weak_ptr_factory_.GetWeakPtr()));
}

void AddToHomescreenDataFetcher::OnFaviconFetched(const SkBitmap& bitmap) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (is_icon_saved_)
    return;

  is_icon_saved_ = true;

  base::android::ScopedJavaLocalRef<jobject> java_bitmap;
  if (bitmap.getSize())
    java_bitmap = gfx::ConvertToJavaBitmap(&bitmap);

  FinalizeAndSetIcon(java_bitmap.obj());
}

void AddToHomescreenDataFetcher::OnIconFetchTimeoutExpired() {
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&AddToHomescreenDataFetcher::OnFaviconFetched,
                 weak_ptr_factory_.GetWeakPtr(), SkBitmap()));
}

bool AddToHomescreenDataFetcher::OnMessageReceived(
    const IPC::Message& message) {
  bool handled = true;

  IPC_BEGIN_MESSAGE_MAP(AddToHomescreenDataFetcher, message)
    IPC_MESSAGE_HANDLER(OperaViewHostMsg_DidGetWebApplicationInfo,
                        OnDidGetWebApplicationInfo)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void AddToHomescreenDataFetcher::WebContentsDestroyed() {
  OnDestroy();
}

}  // namespace opera
