// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA. All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/webapps/add_to_homescreen_data_fetcher.h"
%}
#include "third_party/skia/include/core/SkBitmap.h"

%rename (NativeAddToHomescreenDataFetcher) opera::AddToHomescreenDataFetcher;

%rename("%(regex:/AddToHomescreenDataFetcher::([A-Z].*)/\\l\\1/)s", regextarget=1, fullname=1, %$isfunction) ".*";

SWIG_SELFREF_NAMESPACED_CONSTRUCTOR(opera, AddToHomescreenDataFetcher);

namespace opera {

%feature("director") AddToHomescreenDataFetcher;
class AddToHomescreenDataFetcher {
  public:
    AddToHomescreenDataFetcher(jobject web_contents, int idealIconSize, int minIconSize);
    virtual ~AddToHomescreenDataFetcher();

    void Initialize();
    void TearDown();

    static bool IsInProgress(jobject web_contents);

    virtual void OnInitialized(
      const base::string16& title,
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
};

}  // namespace opera
