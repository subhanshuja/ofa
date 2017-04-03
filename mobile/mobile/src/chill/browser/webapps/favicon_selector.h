// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_WEBAPPS_FAVICON_SELECTOR_H_
#define CHILL_BROWSER_WEBAPPS_FAVICON_SELECTOR_H_

#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "third_party/skia/include/core/SkBitmap.h"

#include "chill/common/webapps/web_application_info.h"

namespace content {
class WebContents;
}  // namespace content

namespace gfx {
class Screen;
}

namespace opera {

class FaviconSelector {
 public:
  explicit FaviconSelector(content::WebContents* web_contents);

  typedef base::Callback<void(const SkBitmap&)> FaviconSelectedCallback;

  void FindBestMatchingIconDP(
      const std::vector<WebApplicationInfo::IconInfo>& icons,
      int ideal_icon_size_in_dp,
      const FaviconSelectedCallback& callback);

  void FindBestMatchingIconPX(
      const std::vector<WebApplicationInfo::IconInfo>& icons,
      int ideal_icon_size_in_px,
      const FaviconSelectedCallback& callback);

 private:
  void ProcessCandidates();
  void DoneProcessingCandidates();

  void OnFaviconDownloaded(int id,
                           int http_status_code,
                           const GURL& url,
                           const std::vector<SkBitmap>& bitmaps,
                           const std::vector<gfx::Size>& sizes);

  void OnFinalFaviconDownloaded(int id,
                                int http_status_code,
                                const GURL& url,
                                const std::vector<SkBitmap>& bitmaps,
                                const std::vector<gfx::Size>& sizes);

  void NotifyClient(const SkBitmap& bitmap);

  content::WebContents* web_contents_;

  bool favicon_selector_in_progress_;
  int ideal_icon_size_in_px_;
  FaviconSelectedCallback callback_;
  std::vector<WebApplicationInfo::IconInfo> candidates_;
  WebApplicationInfo::IconInfo best_candidate_;
  WebApplicationInfo::IconInfo biggest_icon_;

  base::WeakPtrFactory<FaviconSelector> weak_ptr_factory_;
};

}  // namespace opera

#endif  // CHILL_BROWSER_WEBAPPS_FAVICON_SELECTOR_H_
