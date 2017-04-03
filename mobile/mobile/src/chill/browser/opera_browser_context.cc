// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/shell_browser_context.cc
// @final-synchronized

#include "chill/browser/opera_browser_context.h"

#include <string>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/threading/thread.h"
#include "components/visitedlink/browser/visitedlink_master.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/resource_context.h"
#include "content/public/browser/resource_dispatcher_host.h"
#include "content/public/browser/resource_dispatcher_host_delegate.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_client.h"

#include "chill/app/native_interface.h"
#include "chill/browser/background_sync/opera_background_sync_controller.h"
#include "chill/browser/clear_data_machine.h"
#include "chill/browser/download/opera_download_manager_delegate.h"
#include "chill/browser/net/opera_url_request_context_getter.h"
#include "chill/browser/opera_content_browser_client.h"
#include "chill/browser/opera_web_contents_delegate.h"
#include "chill/browser/permissions/opera_permission_manager.h"
#include "chill/browser/push_messaging/opera_push_messaging_service.h"
#include "chill/browser/ssl/opera_ssl_host_state_delegate.h"
#include "chill/common/opera_switches.h"

using content::BrowserThread;

namespace opera {

OperaBrowserContext::OperaBrowserContext(bool off_the_record,
                                         net::NetLog* net_log)
    : off_the_record_(off_the_record),
      net_log_(net_log),
      resource_context_(new OperaResourceContext(off_the_record)) {
  InitWhileIOAllowed();
}

OperaBrowserContext::~OperaBrowserContext() {
  if (resource_context_.get()) {
    BrowserThread::DeleteSoon(BrowserThread::IO, FROM_HERE,
                              resource_context_.release());
  }
}

// static
OperaBrowserContext* OperaBrowserContext::FromWebContents(
    content::WebContents* web_contents) {
  // This is safe; this is the only implementation of the browser context.
  return static_cast<OperaBrowserContext*>(web_contents->GetBrowserContext());
}

// static
OperaBrowserContext* OperaBrowserContext::FromBrowserContext(
    content::BrowserContext* context) {
  // This is safe; this is the only implementation of the browser context.
  return static_cast<OperaBrowserContext*>(context);
}

void OperaBrowserContext::InitWhileIOAllowed() {
  bool got_path = PathService::Get(base::DIR_ANDROID_APP_DATA, &path_);
  DCHECK(got_path);
  if (!base::PathExists(path_))
    base::CreateDirectory(path_);
  BrowserContext::Initialize(this, path_);
}

std::unique_ptr<content::ZoomLevelDelegate>
OperaBrowserContext::CreateZoomLevelDelegate(const base::FilePath&) {
  return std::unique_ptr<content::ZoomLevelDelegate>();
}

base::FilePath OperaBrowserContext::GetPath() const {
  return path_;
}

bool OperaBrowserContext::IsOffTheRecord() const {
  return off_the_record_;
}

content::DownloadManagerDelegate*
OperaBrowserContext::GetDownloadManagerDelegate() {
  content::DownloadManager* manager = BrowserContext::GetDownloadManager(this);

  if (!download_manager_delegate_.get()) {
    download_manager_delegate_.reset(
        opera::NativeInterface::Get()->GetDownloadManagerDelegate(
            GetPrivateBrowserContext() != this));
    download_manager_delegate_->SetDownloadManager(manager);
  }

  return download_manager_delegate_.get();
}

net::URLRequestContextGetter* OperaBrowserContext::CreateRequestContext(
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) {
  DCHECK(!url_request_getter_);
  url_request_getter_ = new OperaURLRequestContextGetter(GetPath(),
      BrowserThread::UnsafeGetMessageLoopForThread(BrowserThread::IO),
      BrowserThread::UnsafeGetMessageLoopForThread(BrowserThread::FILE),
      protocol_handlers, std::move(request_interceptors), off_the_record_,
      net_log_);
  resource_context_->set_url_request_context_getter(url_request_getter_.get());
  return url_request_getter_.get();
}

net::URLRequestContextGetter* OperaBrowserContext::CreateMediaRequestContext() {
  return url_request_getter_.get();
}

net::URLRequestContextGetter*
OperaBrowserContext::CreateRequestContextForStoragePartition(
    const base::FilePath& partition_path,
    bool in_memory,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) {
  return NULL;
}

net::URLRequestContextGetter*
OperaBrowserContext::CreateMediaRequestContextForStoragePartition(
    const base::FilePath& partition_path,
    bool in_memory) {
  return url_request_getter_.get();
}

content::ResourceContext* OperaBrowserContext::GetResourceContext() {
  return resource_context_.get();
}

content::BrowserPluginGuestManager* OperaBrowserContext::GetGuestManager() {
  return NULL;
}

storage::SpecialStoragePolicy* OperaBrowserContext::GetSpecialStoragePolicy() {
  return NULL;
}

content::PushMessagingService* OperaBrowserContext::GetPushMessagingService() {
  if (IsOffTheRecord())
    return NULL;
  if (!push_messaging_service_.get())
    push_messaging_service_.reset(new OperaPushMessagingService(this));
  return push_messaging_service_.get();
}

content::SSLHostStateDelegate* OperaBrowserContext::GetSSLHostStateDelegate() {
  if (!ssl_host_state_delegate_.get())
    ssl_host_state_delegate_.reset(new OperaSSLHostStateDelegate());

  return ssl_host_state_delegate_.get();
}

content::PermissionManager* OperaBrowserContext::GetPermissionManager() {
  if (!permission_manager_.get())
    permission_manager_.reset(new OperaPermissionManager(off_the_record_));
  return permission_manager_.get();
}

content::BackgroundSyncController*
OperaBrowserContext::GetBackgroundSyncController() {
  if (!background_sync_controller_.get())
    background_sync_controller_.reset(new OperaBackgroundSyncController(this));
  return background_sync_controller_.get();
}

void OperaBrowserContext::ClearBrowsingData(OperaBrowserContext* context) {
  ClearDataMachine* machine = ClearDataMachine::CreateClearDataMachine(context);
  machine->ScheduleClearData(ClearDataMachine::CLEAR_ALL);
  machine->Clear();
}

void OperaBrowserContext::ClearBrowsingData() {
  OperaBrowserContext* context = GetDefaultBrowserContext();
  ClearBrowsingData(context);
}

void OperaBrowserContext::ClearPrivateBrowsingData() {
  OperaBrowserContext* context = GetPrivateBrowserContext();
  ClearBrowsingData(context);
}

void OperaBrowserContext::FlushCookieStorage() {
  OperaBrowserContext* context = GetDefaultBrowserContext();

  DCHECK(context->url_request_getter_.get());
  context->url_request_getter_->FlushCookieStorage();
}

void OperaBrowserContext::OnAppDestroy() {
  OperaBrowserContext* context = GetDefaultBrowserContext();

  content::DownloadManager* manager =
      BrowserContext::GetDownloadManager(context);
  manager->Shutdown();

  DCHECK(context->url_request_getter_.get());
  context->url_request_getter_->OnAppDestroy();

  if (context->push_messaging_service_)
    context->push_messaging_service_->Shutdown();
}

OperaBrowserContext* OperaBrowserContext::GetDefaultBrowserContext() {
  return OperaContentBrowserClient::Get()->browser_context();
}

OperaBrowserContext* OperaBrowserContext::GetPrivateBrowserContext() {
  return OperaContentBrowserClient::Get()->off_the_record_browser_context();
}

void OperaBrowserContext::SetResourceDispatcherHostDelegate(
    OperaResourceDispatcherHostDelegate* delegate) {
  content::ResourceDispatcherHost::Get()->SetDelegate(delegate);
  GetDefaultBrowserContext()->resource_dispatcher_host_delegate_.reset(
      delegate);
}

void OperaBrowserContext::SetTurboDelegate(TurboDelegate* delegate) {
  GetDefaultBrowserContext()->turbo_delegate_.reset(delegate);
}

bool OperaBrowserContext::IsHandledUrl(const GURL& url) {
  if (!url_request_getter_)
    return false;
  return url_request_getter_->IsHandledProtocol(url.scheme());
}

std::string OperaBrowserContext::GetAcceptLanguagesHeader() {
  return OperaURLRequestContextGetter::GenerateAcceptLanguagesHeader();
}

}  // namespace opera
