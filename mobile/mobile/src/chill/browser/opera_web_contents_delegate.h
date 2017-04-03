// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_OPERA_WEB_CONTENTS_DELEGATE_H_
#define CHILL_BROWSER_OPERA_WEB_CONTENTS_DELEGATE_H_

#include "base/android/jni_android.h"
#include "base/strings/string16.h"
#include "components/web_contents_delegate_android/web_contents_delegate_android.h"
#include "content/public/browser/invalidate_type.h"
#include "content/public/common/window_container_type.h"
#include "third_party/WebKit/public/platform/WebDisplayMode.h"
#include "third_party/WebKit/public/platform/WebReferrerPolicy.h"
#include "ui/base/window_open_disposition.h"

namespace opera {

class OperaWebContentsDelegate
    : public web_contents_delegate_android::WebContentsDelegateAndroid {
 public:
  OperaWebContentsDelegate(JNIEnv* env,
                           jobject obj)
    : web_contents_delegate_android::WebContentsDelegateAndroid(env, obj)
    , find_current_(0)
    , find_max_(0) {}

  content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params) override;

  void AddNewContents(content::WebContents* source,
                              content::WebContents* new_contents,
                              WindowOpenDisposition disposition,
                              const gfx::Rect& initial_pos,
                              bool user_gesture,
                              bool* was_blocked) override;

  bool ShouldSuppressDialogs(content::WebContents* source) override;

  void DidNavigateMainFramePostCommit(
      content::WebContents* web_contents) override;

  content::JavaScriptDialogManager* GetJavaScriptDialogManager(
      content::WebContents* source) override;

  void RunFileChooser(content::RenderFrameHost* render_frame_host,
                      const content::FileChooserParams& params) override;

  blink::WebDisplayMode GetDisplayMode(
      const content::WebContents* web_contents) const override;

  void FindReply(content::WebContents* web_contents,
                         int request_id,
                         int number_of_matches,
                         const gfx::Rect& selection_rect,
                         int active_match_ordinal,
                         bool final_update) override;

  void RequestMediaAccessPermission(
      content::WebContents* web_contents,
      const content::MediaStreamRequest& request,
      const content::MediaResponseCallback& callback) override;

  std::unique_ptr<content::BluetoothChooser> RunBluetoothChooser(
      content::RenderFrameHost* frame,
      const content::BluetoothChooser::EventHandler& event_handler) override;

 private:
  int find_current_;
  int find_max_;
};

}  // namespace opera

#endif  // CHILL_BROWSER_OPERA_WEB_CONTENTS_DELEGATE_H_
