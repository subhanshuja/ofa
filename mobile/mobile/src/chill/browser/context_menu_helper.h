// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_CONTEXT_MENU_HELPER_H_
#define CHILL_BROWSER_CONTEXT_MENU_HELPER_H_

#include <string>

#include "base/android/scoped_java_ref.h"
#include "content/public/browser/web_contents_user_data.h"
#include "content/public/common/context_menu_params.h"

namespace content {
struct ContextMenuParams;
class WebContents;
}

namespace opera {

class ContextMenuHelper
    : public content::WebContentsUserData<ContextMenuHelper>,
      public base::android::ScopedJavaGlobalRef<jobject> {
 public:
  explicit ContextMenuHelper(jobject j_web_contents);
  virtual ~ContextMenuHelper() {}

  void ShowContextMenu(const content::ContextMenuParams& params);

  // Implemented in Java.
  virtual void ShowContextMenu(blink::WebContextMenuData::MediaType media_type,
                               std::string link_url,
                               std::string link_text,
                               std::string unfiltered_link_url,
                               std::string src_url,
                               std::string keyword_url,
                               std::string selection_text,
                               bool is_editable,
                               std::string sanitized_referrer,
                               blink::WebReferrerPolicy referrer_policy,
                               int x,
                               int y,
                               std::string title) = 0;

 private:
  explicit ContextMenuHelper(content::WebContents* web_contents);
  friend class content::WebContentsUserData<ContextMenuHelper>;

  content::WebContents* web_contents_;
};

}  // namespace opera

#endif  // CHILL_BROWSER_CONTEXT_MENU_HELPER_H_
