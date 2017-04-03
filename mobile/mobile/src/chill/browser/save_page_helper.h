// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_SAVE_PAGE_HELPER_H_
#define CHILL_BROWSER_SAVE_PAGE_HELPER_H_

#include <jni.h>
#include <string>

#include "third_party/WebKit/public/platform/WebReferrerPolicy.h"
#include "url/gurl.h"

namespace opera {

class SavePageHelper {
 public:
  static bool IsSavable(jobject j_web_contents);
  static bool SavePage(jobject j_web_contents, const std::string& file);
  static void SaveFrame(jobject j_web_contents,
                        const GURL& url,
                        const GURL& referrer_url,
                        blink::WebReferrerPolicy referrer_policy);
};

}  // namespace opera

#endif  // CHILL_BROWSER_SAVE_PAGE_HELPER_H_
