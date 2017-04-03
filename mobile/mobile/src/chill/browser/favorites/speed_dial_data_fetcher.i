// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA. All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/favorites/speed_dial_data_fetcher.h"
%}

%rename (NativeSpeedDialDataFetcher) opera::SpeedDialDataFetcher;

%rename("%(regex:/SpeedDialDataFetcher::([A-Z].*)/\\l\\1/)s", regextarget=1, fullname=1, %$isfunction) ".*";

SWIG_SELFREF_NAMESPACED_CONSTRUCTOR(opera, SpeedDialDataFetcher);

namespace opera {

%javamethodmodifiers SpeedDialDataFetcher::Initialize() "protected";
%javamethodmodifiers SpeedDialDataFetcher::FinalizeAndSetIcon(jobject icon) "protected";

%feature("director") SpeedDialDataFetcher;
class SpeedDialDataFetcher {
  public:
    SpeedDialDataFetcher(jobject web_contents, int idealIconSize);
    virtual ~SpeedDialDataFetcher();

    void Initialize();

    static bool IsInProgress(jobject web_contents);

    virtual void FinalizeAndSetIcon(jobject icon) = 0;
};

}  // namespace opera
