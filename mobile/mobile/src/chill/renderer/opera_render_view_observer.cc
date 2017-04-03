// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/renderer/opera_render_view_observer.h"

#include <vector>

#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "third_party/WebKit/public/web/WebView.h"
#include "third_party/WebKit/public/web/WebFrame.h"

#include "chill/common/opera_messages.h"
#include "chill/common/webapps/web_application_info.h"
#include "chill/renderer/webapps/web_apps.h"
#include "third_party/WebKit/public/web/WebConsoleMessage.h"

using blink::WebFrame;
using blink::WebView;

namespace opera {

OperaRenderViewObserver::OperaRenderViewObserver(
    content::RenderView* render_view)
    : content::RenderViewObserver(render_view) {
}

OperaRenderViewObserver::~OperaRenderViewObserver() {
}

void OperaRenderViewObserver::OnDestruct() {
  delete this;
}

bool OperaRenderViewObserver::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(OperaRenderViewObserver, message)
    IPC_MESSAGE_HANDLER(OpViewMsg_GetWebApplicationInfo,
                        OnGetWebApplicationInfo)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void OperaRenderViewObserver::OnGetWebApplicationInfo() {
  WebFrame* main_frame = render_view()->GetWebView()->mainFrame();
  DCHECK(main_frame);

  WebApplicationInfo web_app_info;
  web_apps::ParseWebAppFromWebDocument(main_frame, &web_app_info);

  // Prune out any data URLs in the set of icons.  The browser process expects
  // any icon with a data URL to have originated from a favicon.  We don't want
  // to decode arbitrary data URLs in the browser process.  See
  // http://b/issue?id=1162972
  for (std::vector<WebApplicationInfo::IconInfo>::iterator it =
           web_app_info.icons.begin();
       it != web_app_info.icons.end();) {
    if (it->url.SchemeIs(url::kDataScheme))
      it = web_app_info.icons.erase(it);
    else
      ++it;
  }

  // Truncate the strings we send to the browser process.
  web_app_info.title = web_app_info.title.substr(0, kMaxMetaTagAttributeLength);
  web_app_info.description = web_app_info.description.substr(0, 2000);

  Send(new OperaViewHostMsg_DidGetWebApplicationInfo(routing_id(),
                                                     web_app_info));
}

}  // namespace opera
