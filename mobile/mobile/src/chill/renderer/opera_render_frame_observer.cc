// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/renderer/opera_render_frame_observer.h"

#include <string>

#include "base/strings/utf_string_conversions.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/WebKit/public/platform/modules/app_banner/WebAppBannerPromptReply.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebSecurityPolicy.h"
#include "url/gurl.h"

#include "chill/common/app_banner_messages.h"

OperaRenderFrameObserver::OperaRenderFrameObserver(
    content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {
}

OperaRenderFrameObserver::~OperaRenderFrameObserver() {
}

void OperaRenderFrameObserver::OnDestruct() {
  delete this;
}

bool OperaRenderFrameObserver::OnMessageReceived(const IPC::Message& message) {
  // Filter only.
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(OperaRenderFrameObserver, message)
    IPC_MESSAGE_HANDLER(OperaViewMsg_AppBannerPromptRequest,
                        OnAppBannerPromptRequest)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void OperaRenderFrameObserver::OnAppBannerPromptRequest(
    int request_id,
    const std::string& platform) {
  // App banner prompt requests are handled in the general opera render frame
  // observer, not the AppBannerClient, as the AppBannerClient is created lazily
  // by blink and may not exist when the request is sent.
  blink::WebAppBannerPromptReply reply = blink::WebAppBannerPromptReply::None;
  blink::WebString web_platform(base::UTF8ToUTF16(platform));
  blink::WebVector<blink::WebString> web_platforms(&web_platform, 1);

  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  frame->willShowInstallBannerPrompt(request_id, web_platforms, &reply);

  // Extract the referrer header for this site according to its referrer policy.
  // Pass in an empty URL as the destination so that it is always treated
  // as a cross-origin request.
  std::string referrer = blink::WebSecurityPolicy::generateReferrerHeader(
      frame->document().referrerPolicy(), GURL(),
      frame->document().outgoingReferrer()).utf8();

  Send(new OperaViewHostMsg_AppBannerPromptReply(
      routing_id(), request_id, reply, referrer));
}
