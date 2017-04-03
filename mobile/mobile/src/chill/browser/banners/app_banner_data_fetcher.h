// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/browser/banners/app_baner_data_fetcher.h
// @final-synchronized

#ifndef CHILL_BROWSER_BANNERS_APP_BANNER_DATA_FETCHER_H_
#define CHILL_BROWSER_BANNERS_APP_BANNER_DATA_FETCHER_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/manifest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/WebKit/public/platform/modules/app_banner/WebAppBannerPromptReply.h"

namespace opera {

// Fetches data required to show a web app banner for the URL currently shown by
// the WebContents.
class AppBannerDataFetcher
    : public base::RefCounted<AppBannerDataFetcher>,
      public content::WebContentsObserver {
 public:
  class Delegate {
   public:
    virtual void ShowBanner(const SkBitmap* icon,
                            bool is_showing_first_time) = 0;
  };

  // Returns the current time.
  static base::Time GetCurrentTime();

  AppBannerDataFetcher(content::WebContents* web_contents,
                       const base::WeakPtr<Delegate>& weak_delegate,
                       int ideal_icon_size_in_dp,
                       int minimum_icon_size_in_dp);

  // Begins creating a banner for the URL being displayed by the Delegate's
  // WebContents.
  void Start(const GURL& validated_url, ui::PageTransition transition_type);

  // Stops the pipeline when any asynchronous calls return.
  void Cancel();

  // Returns whether or not the pipeline has been stopped.
  bool is_active() { return is_active_; }

  // Returns whether the beforeinstallprompt Javascript event was canceled.
  bool was_canceled_by_page() { return was_canceled_by_page_; }

  // Returns whether the page has validly requested that the banner be shown
  // by calling prompt() on the beforeinstallprompt Javascript event.
  bool page_requested_prompt() { return page_requested_prompt_; }

  // Returns the type of transition which triggered this fetch.
  ui::PageTransition transition_type() { return transition_type_; }

  // Returns the URL that kicked off the banner data retrieval.
  const GURL& validated_url() { return validated_url_; }

  // Returns the Manifest structure containing webapp information.
  const content::Manifest& web_app_data() { return web_app_data_; }

  // WebContentsObserver overrides.
  void WebContentsDestroyed() override;
  void DidNavigateMainFrame(
      const content::LoadCommittedDetails& details,
      const content::FrameNavigateParams& params) override;
  bool OnMessageReceived(const IPC::Message& message,
                         content::RenderFrameHost* render_frame_host) override;

 private:
  ~AppBannerDataFetcher() override;

  // Return a string describing what type of banner is being created. Used when
  // alerting websites that a banner is about to be created.
  std::string GetBannerType();

  // Called after the manager sends a message to the renderer regarding its
  // intention to show a prompt. The renderer will send a message back with the
  // opportunity to cancel.
  void OnBannerPromptReply(content::RenderFrameHost* render_frame_host,
                           int request_id,
                           blink::WebAppBannerPromptReply reply,
                           std::string referrer);

  // Called when the client has prevented a banner from being shown, and is
  // now requesting that it be shown later.
  void OnRequestShowAppBanner(content::RenderFrameHost* render_frame_host,
                              int request_id);

  // Called when it is determined that the webapp has fulfilled the initial
  // criteria of having a manifest and a service worker.
  void OnHasServiceWorker(content::WebContents* web_contents);

  content::WebContents* GetWebContents();
  std::string GetAppIdentifier();
  int event_request_id() { return event_request_id_; }

  // Fetches the icon at the given URL asynchronously, returning |false| if a
  // load could not be started.
  bool FetchAppIcon(content::WebContents* web_contents, const GURL& url);

  // Records that a banner was shown. The |event_name| corresponds to the RAPPOR
  // metric being recorded.
  void RecordDidShowBanner();

  // Callbacks for data retrieval.
  void OnDidGetManifest(const GURL& manifest_url,
                        const content::Manifest& manifest);
  void OnDidCheckHasServiceWorker(bool has_service_worker);
  void OnAppIconFetched(const SkBitmap& bitmap);

  // Returns whether the webapp was already installed or blocked.
  bool IsWebAppInstalledOrBlocked();

  // Record that the banner could be shown at this point, if the triggering
  // heuristic allowed.
  void RecordCouldShowBanner();

  // Returns whether the banner should be shown.
  bool CheckIfShouldShowBanner();

  // Returns whether the fetcher is active and web contents have not been
  // closed.
  bool CheckFetcherIsStillAlive(content::WebContents* web_contents);

  // Returns whether the given Manifest is following the requirements to show
  // a web app banner.
  static bool IsManifestValidForWebApp(const content::Manifest& manifest);

  base::WeakPtr<Delegate> weak_delegate_;
  const int ideal_icon_size_in_dp_;
  const int minimum_icon_size_in_dp_;
  bool is_active_;
  bool was_canceled_by_page_;
  bool page_requested_prompt_;
  ui::PageTransition transition_type_;
  int event_request_id_;
  std::unique_ptr<SkBitmap> app_icon_;
  std::string referrer_;

  GURL validated_url_;
  content::Manifest web_app_data_;

  friend class base::RefCounted<AppBannerDataFetcher>;
  DISALLOW_COPY_AND_ASSIGN(AppBannerDataFetcher);
};

}  // namespace opera

#endif  // CHILL_BROWSER_BANNERS_APP_BANNER_DATA_FETCHER_H_
