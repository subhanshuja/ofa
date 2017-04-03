// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// NOLINTNEXTLINE(whitespace/line_length)
// @copied-from chromium/src/content/shell/browser/shell_devtools_manager_delegate.cc
// @last-synchronized 1ca7ccffe2459dff9e5bdf294256056d1e78ca90
//
// Modifications:
// - Rename Shell* -> Opera*
// - namespace content -> opera
// - Clean-up #if defined(OS_ANDROID)
// - New front end URL.
// - Custom implementation of:
//     CreateNew[Shell]Target()
// - Implement CreateForTethering for the DevToolsSocketFactory

#include "chill/browser/opera_devtools_manager_delegate.h"

#include <stdint.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/atomicops.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/devtools_frontend_host.h"
#include "content/public/browser/devtools_socket_factory.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "content/public/common/user_agent.h"
#include "net/base/net_errors.h"
#include "net/log/net_log_source.h"
#include "net/socket/tcp_server_socket.h"
#include "ui/base/resource/resource_bundle.h"

#include "content/public/browser/android/devtools_auth.h"
#include "net/socket/unix_domain_server_socket_posix.h"

#include "chill/browser/opera_content_browser_client.h"
#include "chill/common/opera_content_client.h"

namespace opera {

namespace {

const char kFrontEndURL[] =
    "https://devtools.opera.com/inspector/%s/inspector.html";
const char kTetheringSocketName[] = "opera_devtools_tethering_%d_%d";

const int kBackLog = 10;

base::subtle::Atomic32 g_last_used_part;

class UnixDomainServerSocketFactory : public content::DevToolsSocketFactory {
 public:
  explicit UnixDomainServerSocketFactory(const std::string& socket_name)
      : socket_name_(socket_name), last_tethering_socket_(0) {}

 private:
  // content::DevToolsSocketFactory.
  std::unique_ptr<net::ServerSocket> CreateForHttpServer() override {
    std::unique_ptr<net::UnixDomainServerSocket> socket(
        new net::UnixDomainServerSocket(base::Bind(&content::CanUserConnectToDevTools),
                                        true /* use_abstract_namespace */));
    if (socket->BindAndListen(socket_name_, kBackLog) != net::OK)
      return std::unique_ptr<net::ServerSocket>();

    return std::move(socket);
  }

  std::unique_ptr<net::ServerSocket> CreateForTethering(
      std::string* name) override {
    *name = base::StringPrintf(kTetheringSocketName, getpid(),
                               ++last_tethering_socket_);
    std::unique_ptr<net::UnixDomainServerSocket> socket(
        new net::UnixDomainServerSocket(
            base::Bind(&content::CanUserConnectToDevTools), true));
    if (socket->BindAndListen(*name, kBackLog) != net::OK)
      return std::unique_ptr<net::ServerSocket>();

    return std::move(socket);
  }

  std::string socket_name_;
  int last_tethering_socket_;

  DISALLOW_COPY_AND_ASSIGN(UnixDomainServerSocketFactory);
};

std::unique_ptr<content::DevToolsSocketFactory> CreateSocketFactory() {
  return std::unique_ptr<content::DevToolsSocketFactory>(
      new UnixDomainServerSocketFactory(OPERA_DEVTOOLS_SOCKET_NAME));
}

void OnWebContentsCreated(content::WebContents* web_contents) {
  // Need this callback to be defined otherwise a DCHECK will fail in
  // ServiceTabLauncher::OnTabLaunched.
}

}  // namespace

// OperaDevToolsManagerDelegate ----------------------------------------------

// static
int OperaDevToolsManagerDelegate::GetHttpHandlerPort() {
  return base::subtle::NoBarrier_Load(&g_last_used_part);
}

// static
void OperaDevToolsManagerDelegate::StartHttpHandler(
    content::BrowserContext* browser_context) {
  std::string frontend_url = base::StringPrintf(kFrontEndURL, CHR_VERSION);
  content::DevToolsAgentHost::StartRemoteDebuggingServer(
      CreateSocketFactory(),
      frontend_url,
      browser_context->GetPath(),
      base::FilePath(),
      GetProduct(),
      GetUserAgent());
}

// static
void OperaDevToolsManagerDelegate::StopHttpHandler() {
  content::DevToolsAgentHost::StopRemoteDebuggingServer();
}

OperaDevToolsManagerDelegate::OperaDevToolsManagerDelegate(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context) {
}

OperaDevToolsManagerDelegate::~OperaDevToolsManagerDelegate() {
}

scoped_refptr<content::DevToolsAgentHost>
OperaDevToolsManagerDelegate::CreateNewTarget(const GURL& url) {
  OperaContentBrowserClient::Get()->OpenURL(
      browser_context_,
      content::OpenURLParams(url,
                             content::Referrer(),
                             WindowOpenDisposition::NEW_FOREGROUND_TAB,
                             ui::PAGE_TRANSITION_LINK,
                             false),
      base::Bind(&OnWebContentsCreated));

  // We don't have the WebContents yet so return NULL. This is still fine
  // since the page will still be sent to any connected session. The
  // only downside is that we initially send that the page couldn't be
  // loaded, but that isn't handled on the other side in any visual
  // way so the user wouldn't know anything went amiss.
  return nullptr;
}

std::string OperaDevToolsManagerDelegate::GetDiscoveryPageHTML() {
  return std::string();
}

std::string OperaDevToolsManagerDelegate::GetFrontendResource(
    const std::string& path) {
  return content::DevToolsFrontendHost::GetFrontendResource(path).as_string();
}

}  // namespace opera
