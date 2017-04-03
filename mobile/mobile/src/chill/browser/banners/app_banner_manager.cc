// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/browser/banners/app_banner_manager.cc
// @final-synchronized

#include "chill/browser/banners/app_banner_manager.h"

#include <string>

#include "base/android/jni_android.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/origin_util.h"
#include "ui/gfx/android/java_bitmap.h"
#include "ui/display/screen.h"

#include "chill/browser/banners/app_banner_settings_helper.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(opera::AppBannerManager);

namespace opera {

bool AppBannerManager::URLsAreForTheSamePage(const GURL& first,
                                             const GURL& second) {
  return first.GetWithEmptyPath() == second.GetWithEmptyPath() &&
         first.path() == second.path() && first.query() == second.query();
}

AppBannerManager::AppBannerManager(content::WebContents* web_contents,
                                   int ideal_icon_size_in_dp,
                                   int minimum_icon_size_in_dp)
    : content::WebContentsObserver(web_contents),
      ideal_icon_size_in_dp_(ideal_icon_size_in_dp),
      minimum_icon_size_in_dp_(minimum_icon_size_in_dp),
      data_fetcher_(nullptr),
      weak_factory_(this) {
  // Hold on to the web contents to keep alive.
  web_contents->SetUserData(UserDataKey(), this);
}

AppBannerManager::AppBannerManager(jobject j_web_contents,
                                   int ideal_icon_size_in_dp,
                                   int minimum_icon_size_in_dp)
    : AppBannerManager(content::WebContents::FromJavaWebContents(
                           base::android::ScopedJavaLocalRef<jobject>(
                               base::android::AttachCurrentThread(),
                               j_web_contents)),
                       ideal_icon_size_in_dp,
                       minimum_icon_size_in_dp) {}

AppBannerManager::~AppBannerManager() {
  CancelActiveFetcher();
}

void AppBannerManager::DidNavigateMainFrame(
    const content::LoadCommittedDetails& details,
    const content::FrameNavigateParams& params) {
  last_transition_type_ = params.transition;
}

void AppBannerManager::DidFinishLoad(
    content::RenderFrameHost* render_frame_host,
    const GURL& validated_url) {
  if (render_frame_host->GetParent())
    return;

  if (data_fetcher_.get() && data_fetcher_->is_active() &&
      URLsAreForTheSamePage(data_fetcher_->validated_url(), validated_url)) {
    return;
  }

  // A secure origin is required to show banners, so exit early if we see the
  // URL is invalid.
  if (!content::IsOriginSecure(validated_url)) {
    return;
  }

  // Kick off the data retrieval pipeline.
  data_fetcher_ = new AppBannerDataFetcher(web_contents(),
                                           weak_factory_.GetWeakPtr(),
                                           ideal_icon_size_in_dp_,
                                           minimum_icon_size_in_dp_);
  data_fetcher_->Start(validated_url, last_transition_type_);
}

void AppBannerManager::ShowBanner(const SkBitmap* icon,
                                  bool is_showing_first_time) {
  DCHECK(data_fetcher_);

  base::android::ScopedJavaLocalRef<jobject> java_bitmap;
  if (icon->getSize())
    java_bitmap = gfx::ConvertToJavaBitmap(icon);

  ShowBanner(java_bitmap.obj(),
             data_fetcher_->web_app_data().short_name.string(),
             data_fetcher_->web_app_data().name.string(),
             data_fetcher_->web_app_data().start_url,
             data_fetcher_->web_app_data().display,
             data_fetcher_->web_app_data().orientation,
             data_fetcher_->web_app_data().background_color,
             data_fetcher_->web_app_data().theme_color,
             is_showing_first_time);
}

void AppBannerManager::OnBannerAccepted() {
  DCHECK(data_fetcher_);

  AppBannerSettingsHelper::RecordBannerInstallEvent(
      web_contents(), data_fetcher_->web_app_data().start_url.spec());
}

void AppBannerManager::OnBannerRejected(bool never_show_again) {
  DCHECK(data_fetcher_);

  if (never_show_again) {
    AppBannerSettingsHelper::RecordBannerBlockEvent(
        web_contents(), data_fetcher_->web_app_data().start_url.spec());
  }
}

void AppBannerManager::ClearBannerEvents() {
  AppBannerSettingsHelper::ClearBannerEvents();
}

void AppBannerManager::CancelActiveFetcher() {
  if (data_fetcher_.get() != nullptr) {
    data_fetcher_->Cancel();
    data_fetcher_ = nullptr;
  }
}

}  // namespace opera
