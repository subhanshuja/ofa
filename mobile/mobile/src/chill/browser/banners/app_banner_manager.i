// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA. All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/banners/app_banner_manager.h"
%}

%rename (NativeAppBannerManager) opera::AppBannerManager;

%rename("%(regex:/AppBannerManager::([A-Z].*)/\\l\\1/)s", regextarget=1, fullname=1, %$isfunction) ".*";

SWIG_SELFREF_NAMESPACED_CONSTRUCTOR(opera, AppBannerManager);

namespace opera {

%javamethodmodifiers AppBannerManager::OnBannerAccepted() "protected";
%javamethodmodifiers AppBannerManager::OnBannerRejected() "protected";

%feature("director") AppBannerManager;
class AppBannerManager {
  public:
    AppBannerManager(jobject web_contents, int ideal_icon_size_in_dp, int minimum_icon_size_in_dp);
    virtual ~AppBannerManager();

    virtual void ShowBanner(
      jobject icon,
      const base::string16& short_name,
      const base::string16& name,
      GURL url,
      int display_mode,
      int orientation,
      int64_t background_color,
      int64_t theme_color,
      bool is_showing_first_time) = 0;

    void OnBannerAccepted();
    void OnBannerRejected(bool never_show_again);

    static void ClearBannerEvents();
};

}  // namespace opera
