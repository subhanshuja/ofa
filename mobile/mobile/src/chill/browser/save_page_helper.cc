// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/save_page_helper.h"

#include "base/android/jni_android.h"
#include "base/files/file_util.h"
#include "content/public/browser/save_page_type.h"
#include "content/public/browser/web_contents.h"

namespace opera {

/* static */
bool SavePageHelper::IsSavable(jobject j_web_contents) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(
          base::android::ScopedJavaLocalRef<jobject>(
              base::android::AttachCurrentThread(), j_web_contents));
  return web_contents->IsSavable();
}

/* static */
bool SavePageHelper::SavePage(jobject j_web_contents, const std::string& file) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(
          base::android::ScopedJavaLocalRef<jobject>(
              base::android::AttachCurrentThread(), j_web_contents));
  base::FilePath path(FILE_PATH_LITERAL(file));
  content::SavePageType type = content::SAVE_PAGE_TYPE_AS_COMPLETE_HTML;
  if (path.MatchesExtension(".mhtml"))
    type = content::SAVE_PAGE_TYPE_AS_MHTML;
  return web_contents->SavePage(path, path.DirName(), type);
}

/* static */
void SavePageHelper::SaveFrame(jobject j_web_contents,
                               const GURL& url,
                               const GURL& referrer_url,
                               blink::WebReferrerPolicy referrer_policy) {
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(
          base::android::ScopedJavaLocalRef<jobject>(
              base::android::AttachCurrentThread(), j_web_contents));
  web_contents->SaveFrame(url,
                          content::Referrer(referrer_url, referrer_policy));
}

}  // namespace opera
