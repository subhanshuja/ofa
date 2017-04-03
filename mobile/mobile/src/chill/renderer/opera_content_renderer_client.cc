// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/shell_content_renderer_client.cc
// @final-synchronized

#include "chill/renderer/opera_content_renderer_client.h"

#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/render_view.h"
#include "components/visitedlink/renderer/visitedlink_slave.h"
#include "services/shell/public/cpp/interface_registry.h"

#include "common/error_pages/error_page.h"
#include "common/sitepatcher/renderer/op_script_injector.h"
#include "common/sitepatcher/renderer/op_script_store.h"
#include "common/sitepatcher/renderer/op_site_prefs.h"

#include "chill/renderer/opera_render_frame_observer.h"
#include "chill/renderer/opera_render_process_observer.h"
#include "chill/renderer/opera_render_view_observer.h"
#include "chill/renderer/password_form_observer.h"
#include "chill/renderer/banners/app_banner_client.h"
#include "chill/renderer/yandex_promotion/yandex_promotion.h"

namespace blink {
class WebFrame;
class WebPluginParams;
class WebURLError;
class WebURLRequest;
}  // namespace blink

namespace opera {

OperaContentRendererClient::OperaContentRendererClient() {
}

OperaContentRendererClient::~OperaContentRendererClient() {
}

void OperaContentRendererClient::RenderThreadStarted() {
  render_process_observer_.reset(new OperaRenderProcessObserver());

  content::RenderThread* thread = content::RenderThread::Get();

  script_store_.reset(new OpScriptStore());
  site_prefs_.reset(new OpSitePrefs());

  visitedlink_slave_.reset(new visitedlink::VisitedLinkSlave);
  thread->GetInterfaceRegistry()->AddInterface(
      visitedlink_slave_->GetBindCallback());
}

void OperaContentRendererClient::RenderFrameCreated(
    content::RenderFrame* render_frame) {
  new OperaRenderFrameObserver(render_frame);
  new PasswordFormObserver(render_frame);

  if (render_frame->IsMainFrame())
    new YandexPromotion(render_frame);
}

void OperaContentRendererClient::RenderViewCreated(
    content::RenderView* render_view) {
  new OperaRenderViewObserver(render_view);
}

bool OperaContentRendererClient::HasErrorPage(int http_status_code,
                                              std::string* error_domain) {
  return http_status_code >= 400;
}

void OperaContentRendererClient::GetNavigationErrorStrings(
    content::RenderFrame* render_frame,
    const blink::WebURLRequest& failed_request,
    const blink::WebURLError& error,
    std::string* error_html,
    base::string16* error_description) {
  GenerateErrorPage(failed_request, error, error_html, error_description);
}

bool OperaContentRendererClient::OverrideCreatePlugin(
    content::RenderFrame* render_frame,
    blink::WebLocalFrame* frame,
    const blink::WebPluginParams& params,
    blink::WebPlugin** plugin) {
  return false;
}

// NOLINTNEXTLINE(runtime/int)
unsigned long long OperaContentRendererClient::VisitedLinkHash(
    const char* canonical_url,
    size_t length) {
  return visitedlink_slave_->ComputeURLFingerprint(canonical_url, length);
}

// NOLINTNEXTLINE(runtime/int)
bool OperaContentRendererClient::IsLinkVisited(unsigned long long link_hash) {
  return visitedlink_slave_->IsVisited(link_hash);
}

std::unique_ptr<blink::WebAppBannerClient>
OperaContentRendererClient::CreateAppBannerClient(
    content::RenderFrame* render_frame) {
  return std::unique_ptr<blink::WebAppBannerClient>(
      new AppBannerClient(render_frame));
}

void OperaContentRendererClient::RunScriptsAtDocumentStart(
    content::RenderFrame* render_frame) {
  // When using cross process frame we can get this call too early.
  // See Chromium FIXME in WebRemoteFrameImpl::createLocalChild().
  blink::WebLocalFrame* web_frame = render_frame->GetWebFrame();
  if (web_frame)
    op_script_injector_utils::RunScriptsAtDocumentStart(web_frame);
}

bool OperaContentRendererClient::ShouldUseMediaPlayerForURL(const GURL& url) {
  // Force the usage of Android MediaPlayer instead of Chrome's
  // internal player. Enables codec support from the platform but
  // disqualifies us from some of the advantages of the unified media
  // pipeline.
  //
  // See https://docs.google.com/document/d/1QB4RVu0zT54Bys7dUHU6QeR3DyjOs-wk-MN9tjEzquQ/view
  //
  // TODO(davve): This is likely a stopgap measure. The Android
  // specific MediaPlayer will likely lag behind featurewise and may
  // be removed in the future.
  //
  // See https://docs.google.com/document/d/1PzxfmHX57A3AzJIip_KI88cTTomNbrBbpTbKB0eQa9w/edit
  return true;
}


}  // namespace opera
