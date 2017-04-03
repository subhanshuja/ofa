// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/context_menu_helper.h"

#include "base/android/jni_android.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/android/content_view_core.h"
#include "content/public/common/context_menu_params.h"
#include "ui/gfx/geometry/point.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(opera::ContextMenuHelper);

namespace opera {

ContextMenuHelper::ContextMenuHelper(content::WebContents* web_contents) :
    web_contents_(web_contents) {
  web_contents->SetUserData(UserDataKey(), this);
}

ContextMenuHelper::ContextMenuHelper(jobject j_web_contents)
    : ContextMenuHelper(content::WebContents::FromJavaWebContents(
          base::android::ScopedJavaLocalRef<jobject>(
              base::android::AttachCurrentThread(),
              j_web_contents))) {}


void ContextMenuHelper::ShowContextMenu(
    const content::ContextMenuParams& params) {
  GURL sanitized_referrer = (params.frame_url.is_empty() ?
      params.page_url : params.frame_url).GetAsReferrer();

  ShowContextMenu(params.media_type,
                  params.link_url.spec(),
                  UTF16ToUTF8(params.link_text),
                  params.unfiltered_link_url.spec(),
                  params.src_url.spec(),
                  params.keyword_url.spec(),
                  UTF16ToUTF8(params.selection_text),
                  params.is_editable,
                  sanitized_referrer.spec(),
                  params.referrer_policy,
                  params.x, params.y,
                  UTF16ToUTF8(web_contents_->GetTitle()));
}

}  // namespace opera
