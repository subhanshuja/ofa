// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/shell_content_renderer_client.h
// @final-synchronized

#ifndef CHILL_RENDERER_OPERA_CONTENT_RENDERER_CLIENT_H_
#define CHILL_RENDERER_OPERA_CONTENT_RENDERER_CLIENT_H_

#include <string>

#include "base/compiler_specific.h"
#include "content/public/renderer/content_renderer_client.h"

class OpScriptStore;
class OpSitePrefs;

namespace blink {
class WebFrame;
class WebLocalFrame;
class WebPlugin;
struct WebPluginParams;
}

namespace visitedlink {
class VisitedLinkSlave;
}

namespace content {
class RenderView;
class RenderFrame;
}

namespace opera {
class OperaRenderProcessObserver;

class OperaContentRendererClient : public content::ContentRendererClient {
 public:
  OperaContentRendererClient();
  virtual ~OperaContentRendererClient();
  void RenderThreadStarted() override;
  void RenderFrameCreated(content::RenderFrame* render_frame) override;
  void RenderViewCreated(content::RenderView* render_view) override;
  bool OverrideCreatePlugin(
      content::RenderFrame* render_frame,
      blink::WebLocalFrame* frame,
      const blink::WebPluginParams& params,
      blink::WebPlugin** plugin) override;

  bool HasErrorPage(int http_status_code, std::string* error_domain) override;

  void GetNavigationErrorStrings(
      content::RenderFrame* render_frame,
      const blink::WebURLRequest& failed_request,
      const blink::WebURLError& error,
      std::string* error_html,
      base::string16* error_description) override;

  // NOLINTNEXTLINE(runtime/int)
  unsigned long long VisitedLinkHash(const char* canonical_url,
                                     size_t length) override;
  // NOLINTNEXTLINE(runtime/int)
  bool IsLinkVisited(unsigned long long link_hash) override;

  std::unique_ptr<blink::WebAppBannerClient> CreateAppBannerClient(
      content::RenderFrame* render_frame) override;

  void RunScriptsAtDocumentStart(content::RenderFrame* render_frame) override;
  bool ShouldUseMediaPlayerForURL(const GURL& url) override;

 private:
  std::unique_ptr<OperaRenderProcessObserver> render_process_observer_;
  std::unique_ptr<OpScriptStore> script_store_;
  std::unique_ptr<OpSitePrefs> site_prefs_;
  std::unique_ptr<visitedlink::VisitedLinkSlave> visitedlink_slave_;
};

}  // namespace opera

#endif  // CHILL_RENDERER_OPERA_CONTENT_RENDERER_CLIENT_H_
