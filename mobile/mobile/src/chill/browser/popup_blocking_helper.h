// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_POPUP_BLOCKING_HELPER_H_
#define CHILL_BROWSER_POPUP_BLOCKING_HELPER_H_

#include <string>

#include "base/android/scoped_java_ref.h"
#include "content/public/browser/web_contents_user_data.h"

#include "chill/browser/blocked_window_params.h"

namespace content {
class WebContents;
}

namespace opera {
class BlockedWindowParams;

class PopupBlockingHelper
    : public content::WebContentsUserData<PopupBlockingHelper>,
      public base::android::ScopedJavaGlobalRef<jobject> {
 public:
  explicit PopupBlockingHelper(jobject j_web_contents);
  virtual ~PopupBlockingHelper() {}

  void HandleBlockedPopup(const BlockedWindowParams& params);

  // Implemented in Java.
  virtual void OnPopupBlocked(const GURL& url) = 0;

 private:
  explicit PopupBlockingHelper(content::WebContents* web_contents);
  friend class content::WebContentsUserData<PopupBlockingHelper>;
};

}  // namespace opera

#endif  // CHILL_BROWSER_POPUP_BLOCKING_HELPER_H_
