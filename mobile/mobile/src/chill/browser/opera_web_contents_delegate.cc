// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/opera_web_contents_delegate.h"

#include <jni.h>

#include "base/android/jni_android.h"
#include "content/browser/frame_host/navigation_entry_impl.h"
#include "content/public/browser/invalidate_type.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/page_type.h"

#include "common/settings/settings_manager.h"

#include "chill/browser/file_select_helper.h"
#include "chill/browser/media_capture_devices_dispatcher.h"
#include "chill/browser/opera_javascript_dialog_manager.h"
#include "chill/browser/chromium_tab.h"
#include "chill/browser/chromium_tab_delegate.h"
#include "chill/browser/ui/opera_bluetooth_chooser_android.h"

using content::BluetoothChooser;
using content::WebContents;

namespace gfx {
class Point;
}  // namespace gfx

namespace opera {

WebContents* OperaWebContentsDelegate::OpenURLFromTab(
    WebContents* source,
    const content::OpenURLParams& openUrlParams) {
  content::NavigationController::LoadURLParams loadUrlParams(openUrlParams.url);
  loadUrlParams.referrer = openUrlParams.referrer;
  loadUrlParams.transition_type = openUrlParams.transition;
  loadUrlParams.override_user_agent =
      SettingsManager::GetUseDesktopUserAgent()
          ? content::NavigationController::UA_OVERRIDE_TRUE
          : content::NavigationController::UA_OVERRIDE_FALSE;

  switch (openUrlParams.disposition) {
    case WindowOpenDisposition::CURRENT_TAB: {
      source->GetController().LoadURLWithParams(loadUrlParams);
      break;
    }
    default: {
      NOTIMPLEMENTED() << ". Tracked by: WAM-695.";
    }
  }

  return source;
}

void OperaWebContentsDelegate::AddNewContents(
    WebContents* source,
    WebContents* new_contents,
    WindowOpenDisposition disposition,
    const gfx::Rect& initial_pos,
    bool user_gesture,
    bool* was_blocked) {
  if (disposition != WindowOpenDisposition::NEW_BACKGROUND_TAB)
    ChromiumTab::FromWebContents(new_contents)->Focus();
}

bool OperaWebContentsDelegate::ShouldSuppressDialogs(WebContents* source) {
  DCHECK(GetJavaScriptDialogManager(source));
  return static_cast<OperaJavaScriptDialogManager*>(
      GetJavaScriptDialogManager(source))->GetSuppressMessages();
}

void OperaWebContentsDelegate::DidNavigateMainFramePostCommit(
    WebContents* web_contents) {
  ChromiumTab::FromWebContents(web_contents)->Navigated(web_contents);
}

content::JavaScriptDialogManager*
OperaWebContentsDelegate::GetJavaScriptDialogManager(WebContents* source) {
  return ChromiumTab::FromWebContents(source)->GetJavaScriptDialogManager();
}

void OperaWebContentsDelegate::RunFileChooser(
    content::RenderFrameHost* render_frame_host,
    const content::FileChooserParams& params) {
  FileSelectHelper::RunFileChooser(render_frame_host, params);
}

blink::WebDisplayMode OperaWebContentsDelegate::GetDisplayMode(
    const content::WebContents* web_contents) const {
  int m = ChromiumTabDelegate::FromWebContents(web_contents)->GetDisplayMode();
  return static_cast<blink::WebDisplayMode>(m);
}

void OperaWebContentsDelegate::FindReply(WebContents* web_contents,
                                         int request_id,
                                         int number_of_matches,
                                         const gfx::Rect& selection_rect,
                                         int active_match_ordinal,
                                         bool final_update) {
  // If number_of_matches or active_match_ordinal is set to -1, use old
  // saved value, otherwise save and use the new value.
  find_current_ = active_match_ordinal >= 0 ? active_match_ordinal
      : find_current_;
  find_max_ = number_of_matches >= 0 ? number_of_matches : find_max_;

  if (final_update) {
    ChromiumTabDelegate::FromWebContents(web_contents)
        ->FindReply(request_id, find_current_, find_max_);
  }
}

void OperaWebContentsDelegate::RequestMediaAccessPermission(
    WebContents* web_contents,
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback) {
  MediaCaptureDevicesDispatcher::GetInstance()->ProcessMediaAccessRequest(
      web_contents, request, callback);
}

std::unique_ptr<BluetoothChooser> OperaWebContentsDelegate::RunBluetoothChooser(
    content::RenderFrameHost* frame,
    const BluetoothChooser::EventHandler& event_handler) {
  return base::WrapUnique(new BluetoothChooserAndroid(frame, event_handler));
}

}  // namespace opera
