// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/opera_web_contents_observer.h"

#include "content/browser/frame_host/interstitial_page_impl.h"
#include "content/common/view_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"

#include "chill/browser/chromium_tab_delegate.h"

namespace opera {

OperaWebContentsObserver::OperaWebContentsObserver(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      web_contents_(web_contents) {
}

OperaWebContentsObserver::~OperaWebContentsObserver() {
}

void OperaWebContentsObserver::UpdatePlayState(
      blink::WebAudioElementPlayState play_state) {
  ChromiumTabDelegate::FromWebContents(web_contents_)
      ->UpdatePlayState(play_state);
}

void OperaWebContentsObserver::DidAttachInterstitialPage() {
  content::RenderViewHost* host =
      web_contents_->GetInterstitialPage()->GetMainFrame()->GetRenderViewHost();
  // Force show the top controls for interstitial. Relying on
  // web_contents->UpdateTopControlsState will not work since it will
  // not update the state for the interstitial but the state for the
  // web contents belonging to the page.
  host->Send(new ViewMsg_UpdateTopControlsState(host->GetRoutingID(),
                                                false,    // enable_hiding,
                                                true,     // enable_showing,
                                                false));  // animate));
}

}  // namespace opera
