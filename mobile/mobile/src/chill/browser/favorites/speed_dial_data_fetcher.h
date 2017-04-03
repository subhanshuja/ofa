// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_FAVORITES_SPEED_DIAL_DATA_FETCHER_H_
#define CHILL_BROWSER_FAVORITES_SPEED_DIAL_DATA_FETCHER_H_

#include <string>

#include "base/android/scoped_java_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "third_party/skia/include/core/SkBitmap.h"

#include "chill/browser/webapps/favicon_selector.h"

namespace opera {

struct WebApplicationInfo;

class SpeedDialDataFetcher
    : public content::WebContentsObserver,
      public content::WebContentsUserData<SpeedDialDataFetcher>,
      public base::android::ScopedJavaGlobalRef<jobject> {
 public:
  SpeedDialDataFetcher(content::WebContents* web_contents,
                       int ideal_icon_size_in_px);
  SpeedDialDataFetcher(jobject j_web_contents, int ideal_icon_size_in_px);

  // Initialize the data fetcher by requesting the information about the page to
  // the renderer process. The initialization is asynchronous and
  // OnDidGetWebApplicationInfo is expected to be called when finished.
  void Initialize();

  void TearDown();

  // Checks if there is a SpeedDialDataFetcher instance already running
  // for the give web contents.
  static bool IsInProgress(jobject j_web_contents);

 private:
  virtual void FinalizeAndSetIcon(jobject icon) = 0;

  // IPC message received when the initialization is finished.
  void OnDidGetWebApplicationInfo(const WebApplicationInfo& web_app_info);

  void OnFaviconFetched(const SkBitmap& bitmap);

  void OnIconFetchTimeoutExpired();

  // WebContentsObserver
  bool OnMessageReceived(const IPC::Message& message) override;

  FaviconSelector favicon_selector_;
  bool did_get_web_app_info_;
  bool is_icon_saved_;
  base::Timer icon_timeout_timer_;
  const int ideal_icon_size_in_px_;

  base::WeakPtrFactory<SpeedDialDataFetcher> weak_ptr_factory_;
};

}  // namespace opera

#endif  // CHILL_BROWSER_FAVORITES_SPEED_DIAL_DATA_FETCHER_H_
