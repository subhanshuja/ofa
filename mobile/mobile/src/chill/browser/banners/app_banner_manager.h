// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/browser/banners/app_banner_manager.h
// @final-synchronized

#ifndef CHILL_BROWSER_BANNERS_APP_BANNER_MANAGER_H_
#define CHILL_BROWSER_BANNERS_APP_BANNER_MANAGER_H_

#include "base/android/scoped_java_ref.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

#include "chill/browser/banners/app_banner_data_fetcher.h"

namespace opera {

/**
 * Creates an app banner.
 *
 * Hooks the wiring together for getting the data for a particular app.
 * Monitors at most one app at a time, tracking the info for the most recently
 * requested app. Any work in progress for other apps is discarded.
 */
class AppBannerManager : public AppBannerDataFetcher::Delegate,
                         public content::WebContentsObserver,
                         public content::WebContentsUserData<AppBannerManager>,
                         public base::android::ScopedJavaGlobalRef<jobject> {
 public:
  AppBannerManager(content::WebContents* web_contents,
                   int ideal_icon_size_in_dp,
                   int minimum_icon_size_in_dp);
  AppBannerManager(jobject j_web_contents,
                   int ideal_icon_size_in_dp,
                   int minimum_icon_size_in_dp);
  ~AppBannerManager() override;

  // Returns whether or not the URLs match for everything except for the ref.
  static bool URLsAreForTheSamePage(const GURL& first, const GURL& second);

  // WebContentsObserver overrides.
  void DidNavigateMainFrame(
      const content::LoadCommittedDetails& details,
      const content::FrameNavigateParams& params) override;
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;

  // AppBannerDataFetcher::Delegate overrides.
  void ShowBanner(const SkBitmap* icon, bool is_showing_first_time) override;

  void OnBannerAccepted();
  void OnBannerRejected(bool never_show_again);

  static void ClearBannerEvents();

 private:
  // Creates a banner for the app using the given info. Implemented in java.
  virtual void ShowBanner(jobject icon,
                          const base::string16& short_name,
                          const base::string16& name,
                          GURL url,
                          int display_mode,
                          int orientation,
                          int64_t background_color,
                          int64_t theme_color,
                          bool is_showing_first_time) = 0;

  // Cancels an active DataFetcher, stopping its banners from appearing.
  void CancelActiveFetcher();

  const int ideal_icon_size_in_dp_;
  const int minimum_icon_size_in_dp_;

  // The type of navigation made to the page
  ui::PageTransition last_transition_type_;

  // Fetches the data required to display a banner for the current page.
  scoped_refptr<AppBannerDataFetcher> data_fetcher_;

  // A weak pointer is used as the lifetime of the ServiceWorkerContext is
  // longer than the lifetime of this banner manager. The banner manager
  // might be gone when calls sent to the ServiceWorkerContext are completed.
  base::WeakPtrFactory<AppBannerManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AppBannerManager);
};

}  // namespace opera

#endif  // CHILL_BROWSER_BANNERS_APP_BANNER_MANAGER_H_
