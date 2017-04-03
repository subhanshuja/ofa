// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/favorites/thumbnail_request_interceptor.h"
%}

%rename("%(regex:/ThumbnailRequestInterceptor::([A-Z].*)/\\l\\1/)s", regextarget=1, fullname=1, %$isfunction) ".*";

namespace opera {

%nodefaultctor ThumbnailRequestInterceptor;
%nodefaultdtor ThumbnailRequestInterceptor;
class ThumbnailRequestInterceptor {
  public:
    static ThumbnailRequestInterceptor* GetInstance();

    void SetThumbnailForUrl(jobject bitmap, const std::string& title, const std::string& url);

    bool HasDeferredRequest(const std::string& url);
};

}  // namespace opera
