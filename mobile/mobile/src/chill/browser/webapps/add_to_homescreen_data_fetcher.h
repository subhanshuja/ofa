// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/browser/android/webapps/add_to_homescreen_data_fetcher.h
// @final-synchronized

#ifndef CHILL_BROWSER_WEBAPPS_ADD_TO_HOMESCREEN_DATA_FETCHER_H_
#define CHILL_BROWSER_WEBAPPS_ADD_TO_HOMESCREEN_DATA_FETCHER_H_

#include <string>

#include "base/android/scoped_java_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/common/manifest.h"
#include "third_party/skia/include/core/SkBitmap.h"

#include "chill/browser/webapps/favicon_selector.h"
#include "chill/common/webapps/web_application_info.h"

namespace opera {

class AddToHomescreenDataFetcher
    : public content::WebContentsObserver,
      public content::WebContentsUserData<AddToHomescreenDataFetcher>,
      public base::android::ScopedJavaGlobalRef<jobject> {
 public:
  AddToHomescreenDataFetcher(content::WebContents* web_contents,
                             int ideal_icon_size_in_dp,
                             int minimum_icon_size_in_dp);
  AddToHomescreenDataFetcher(jobject j_web_contents,
                             int ideal_icon_size_in_dp,
                             int minimum_icon_size_in_dp);

  // Initialize the data fetcher by requesting the information about the page to
  // the renderer process. The initialization is asynchronous and
  // OnDidRetrieveWebappInformation is expected to be called when finished.
  void Initialize();

  void TearDown();

  // Checks if there is a AddToHomescreenDataFetcher instance already running
  // for the give web contents.
  static bool IsInProgress(jobject j_web_contents);

  // IPC message received when the initialization is finished.
  void OnDidGetWebApplicationInfo(const WebApplicationInfo& web_app_info);

  virtual void OnInitialized(const base::string16& title,
                             const base::string16& short_name,
                             const base::string16& name,
                             GURL url,
                             int display_mode,
                             int orientation,
                             bool mobile_capable,
                             int64_t background_color,
                             int64_t theme_color) = 0;

  virtual void FinalizeAndSetIcon(jobject icon) = 0;

  virtual void SetIcon(jobject icon) = 0;

  virtual void OnDestroy() = 0;

  // WebContentsObserver
  bool OnMessageReceived(const IPC::Message& message) override;
  void WebContentsDestroyed() override;

 private:
  void OnDidGetManifest(const GURL& manifest_url,
                        const content::Manifest& manifest);
  void OnManifestIconFetched(const SkBitmap& icon_bitmap);

  // Grabs the favicon for the wabapp info.
  void FetchFavicon();
  void OnFaviconFetched(const SkBitmap& bitmap);

  void OnIconFetchTimeoutExpired();

  WebApplicationInfo web_app_info_;
  base::string16 title_;
  base::string16 short_name_;
  base::string16 name_;
  GURL url_;

  FaviconSelector favicon_selector_;
  bool did_get_web_app_info_;
  bool is_icon_saved_;
  base::Timer icon_timeout_timer_;
  const int ideal_icon_size_in_dp_;
  const int minimum_icon_size_in_dp_;

  base::WeakPtrFactory<AddToHomescreenDataFetcher> weak_ptr_factory_;
};

}  // namespace opera

#endif  // CHILL_BROWSER_WEBAPPS_ADD_TO_HOMESCREEN_DATA_FETCHER_H_
