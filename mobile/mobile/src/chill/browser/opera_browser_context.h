// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/shell_browser_context.h
// @final-synchronized

#ifndef CHILL_BROWSER_OPERA_BROWSER_CONTEXT_H_
#define CHILL_BROWSER_OPERA_BROWSER_CONTEXT_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/content_browser_client.h"

#include "chill/browser/net/turbo_delegate.h"
#include "chill/browser/opera_resource_context.h"
#include "chill/browser/opera_resource_dispatcher_host_delegate.h"

namespace content {
class BackgroundSyncController;
class DownloadManagerDelegate;
class ResourceContext;
class WebContents;
}

namespace opera {

class OperaBackgroundSyncController;
class OperaDownloadManagerDelegate;
class OperaPermissionManager;
class OperaPushMessagingService;
class OperaSSLHostStateDelegate;
class OperaURLRequestContextGetter;

class OperaBrowserContext : public content::BrowserContext {
 public:
  explicit OperaBrowserContext(bool off_the_record, net::NetLog* net_log);
  virtual ~OperaBrowserContext();

  // Convenience method to return the OperaBrowserContext
  // corresponding to the given WebContents.
  static OperaBrowserContext* FromWebContents(
      content::WebContents* web_contents);
  static OperaBrowserContext* FromBrowserContext(
      content::BrowserContext* context);

  // BrowserContext implementation.
  std::unique_ptr<content::ZoomLevelDelegate> CreateZoomLevelDelegate(
      const base::FilePath& partition_path) override;
  base::FilePath GetPath() const override;
  bool IsOffTheRecord() const override;
  content::DownloadManagerDelegate* GetDownloadManagerDelegate() override;
  content::ResourceContext* GetResourceContext() override;
  content::BrowserPluginGuestManager* GetGuestManager() override;
  storage::SpecialStoragePolicy* GetSpecialStoragePolicy() override;
  content::PushMessagingService* GetPushMessagingService() override;
  content::SSLHostStateDelegate* GetSSLHostStateDelegate() override;
  content::PermissionManager* GetPermissionManager() override;
  content::BackgroundSyncController* GetBackgroundSyncController() override;

  net::URLRequestContextGetter* CreateRequestContext(
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors);
  net::URLRequestContextGetter* CreateMediaRequestContext() override;
  net::URLRequestContextGetter* CreateRequestContextForStoragePartition(
      const base::FilePath& partition_path,
      bool in_memory,
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors) override;
  net::URLRequestContextGetter* CreateMediaRequestContextForStoragePartition(
      const base::FilePath& partition_path,
      bool in_memory) override;
  net::NetLog* net_log() const { return net_log_; }

  // Opera extensions
  static void ClearBrowsingData();
  static void ClearBrowsingData(OperaBrowserContext* context);
  static void ClearPrivateBrowsingData();
  static void FlushCookieStorage();
  static void OnAppDestroy();
  static OperaBrowserContext* GetDefaultBrowserContext();
  static OperaBrowserContext* GetPrivateBrowserContext();

  // Assumes ownership over the delegate
  static void SetResourceDispatcherHostDelegate(
      OperaResourceDispatcherHostDelegate* delegate);

  static void SetTurboDelegate(TurboDelegate* delegate);

  bool IsHandledUrl(const GURL& url);

  TurboDelegate* turbo_delegate() { return turbo_delegate_.get(); }

  static std::string GetAcceptLanguagesHeader();

 private:
  // Performs initialization of the OperaBrowserContext while IO is still
  // allowed on the current thread.
  void InitWhileIOAllowed();

  bool off_the_record_;
  net::NetLog* net_log_;
  base::FilePath path_;
  std::unique_ptr<TurboDelegate> turbo_delegate_;
  std::unique_ptr<OperaBackgroundSyncController> background_sync_controller_;
  std::unique_ptr<OperaResourceContext> resource_context_;
  std::unique_ptr<OperaResourceDispatcherHostDelegate>
      resource_dispatcher_host_delegate_;
  std::unique_ptr<OperaPushMessagingService> push_messaging_service_;
  std::unique_ptr<OperaSSLHostStateDelegate> ssl_host_state_delegate_;
  std::unique_ptr<OperaDownloadManagerDelegate> download_manager_delegate_;
  std::unique_ptr<OperaPermissionManager> permission_manager_;
  scoped_refptr<OperaURLRequestContextGetter> url_request_getter_;

  DISALLOW_COPY_AND_ASSIGN(OperaBrowserContext);
};

}  // namespace opera

#endif  // CHILL_BROWSER_OPERA_BROWSER_CONTEXT_H_
