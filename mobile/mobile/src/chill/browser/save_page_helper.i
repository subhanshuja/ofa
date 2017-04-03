// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/save_page_helper.h"
%}

namespace opera {

%rename(NativeSavePageHelper) SavePageHelper;

class SavePageHelper {
 public:
  static bool IsSavable(jobject j_web_contents);
  static bool SavePage(jobject j_web_contents, const std::string& path);
  static void SaveFrame(jobject j_web_contents,
                        GURL& url,
                        GURL& referrer_url,
                        blink::WebReferrerPolicy referrer_policy);
};

}  // namespace opera
