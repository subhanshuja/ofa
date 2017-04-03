// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/find_in_page.h"

#include "base/android/jni_android.h"
#include "content/public/browser/web_contents.h"
#include "third_party/WebKit/public/web/WebFindOptions.h"

namespace opera {

FindInPage::FindInPage(jobject j_web_contents) : web_contents_(nullptr) {
  web_contents_ = content::WebContents::FromJavaWebContents(
      base::android::ScopedJavaLocalRef<jobject>(
          base::android::AttachCurrentThread(), j_web_contents));
}

void FindInPage::Find(int request_id,
                      const base::string16& term,
                      bool forward,
                      bool match_case,
                      bool find_next) {
  blink::WebFindOptions options;

  options.forward = forward;
  options.matchCase = match_case;
  options.findNext = find_next;

  web_contents_->Find(request_id, term, options);
}

void FindInPage::Cancel() {
  web_contents_->StopFinding(content::STOP_FIND_ACTION_CLEAR_SELECTION);
}

}  // namespace opera
