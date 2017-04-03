// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/popup_blocking_helper.h"

#include "base/android/jni_android.h"
#include "content/public/browser/android/content_view_core.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(opera::PopupBlockingHelper);

namespace opera {

PopupBlockingHelper::PopupBlockingHelper(content::WebContents* web_contents) {
  web_contents->SetUserData(UserDataKey(), this);
}

PopupBlockingHelper::PopupBlockingHelper(jobject j_web_contents)
    : PopupBlockingHelper(content::WebContents::FromJavaWebContents(
          base::android::ScopedJavaLocalRef<jobject>(
              base::android::AttachCurrentThread(),
              j_web_contents))) {}

void PopupBlockingHelper::HandleBlockedPopup(
    const BlockedWindowParams& params) {
  OnPopupBlocked(params.target_url());
}

}  // namespace opera
