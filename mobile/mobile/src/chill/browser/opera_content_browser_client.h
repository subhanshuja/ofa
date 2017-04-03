// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from
//    chromium/src/content/shell/browser/shell_content_browser_client.h
// @final-synchronized

#ifndef CHILL_BROWSER_OPERA_CONTENT_BROWSER_CLIENT_H_
#define CHILL_BROWSER_OPERA_CONTENT_BROWSER_CLIENT_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "content/public/browser/content_browser_client.h"
#include "third_party/WebKit/public/platform/modules/permissions/permission_status.mojom.h"

#include "chill/browser/flags_json_storage.h"
#include "chill/browser/opera_resource_dispatcher_host_delegate.h"
#include "chill/browser/push_messaging/push_messaging_json_storage.h"

namespace content {
class BrowserURLHandler;
class DevToolsManagerDelegate;
class QuotaPermissionContext;
}

namespace opera {

class OperaBrowserContext;
class OperaBrowserMainParts;

class OperaContentBrowserClient : public content::ContentBrowserClient {
 public:
  // Gets the current instance.
  static OperaContentBrowserClient* Get();

  OperaContentBrowserClient();
  virtual ~OperaContentBrowserClient();

  // ContentBrowserClient overrides.
  void BrowserURLHandlerCreated(content::BrowserURLHandler* handler) override;
  content::BrowserMainParts* CreateBrowserMainParts(
      const content::MainFunctionParams& parameters) override;
  void AppendExtraCommandLineSwitches(base::CommandLine* command_line,
                                      int child_process_id) override;
  void OverrideWebkitPrefs(content::RenderViewHost* render_view_host,
                           content::WebPreferences* prefs) override;

  content::PlatformNotificationService*
      GetPlatformNotificationService() override;

  bool CanCreateWindow(const GURL& opener_url,
                       const GURL& opener_top_level_frame_url,
                       const GURL& source_origin,
                       WindowContainerType container_type,
                       const GURL& target_url,
                       const content::Referrer& referrer,
                       const std::string& frame_name,
                       WindowOpenDisposition disposition,
                       const blink::WebWindowFeatures& features,
                       bool user_gesture,
                       bool opener_suppressed,
                       content::ResourceContext* context,
                       int render_process_id,
                       int opener_render_view_id,
                       int opener_render_frame_id,
                       bool* no_javascript_access) override;
  void ResourceDispatcherHostCreated() override;
  std::string GetDefaultDownloadName() override;
  base::FilePath GetDefaultDownloadDirectory() override;
  content::WebContentsViewDelegate* GetWebContentsViewDelegate(
      content::WebContents* web_contents) override;
  content::QuotaPermissionContext* CreateQuotaPermissionContext() override;
  net::NetLog* GetNetLog() override;

  void AllowCertificateError(
      content::WebContents* web_contents,
      int cert_error,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      content::ResourceType resource_type,
      bool overridable,
      bool strict_enforcement,
      bool expired_previous_decision,
      const base::Callback<
          void(content::CertificateRequestResultType)>& callback) override;

  void AddCertificate(net::CertificateMimeType cert_type,
                      const void* cert_data,
                      size_t cert_size,
                      int render_process_id,
                      int render_frame_id) override;

  void OpenURL(content::BrowserContext* browser_context,
               const content::OpenURLParams& params,
               const base::Callback<void(content::WebContents*)>& callback)
      override;

  ScopedVector<content::NavigationThrottle> CreateThrottlesForNavigation(
      content::NavigationHandle* navigation_handle) override;

  void GetAdditionalMappedFilesForChildProcess(
      const base::CommandLine& command_line,
      int child_process_id,
      content::FileDescriptorInfo* mappings,
      std::map<int, base::MemoryMappedFile::Region>* regions) override;

  content::MediaObserver* GetMediaObserver() override;

  std::string GetApplicationLocale() override;

  content::DevToolsManagerDelegate* GetDevToolsManagerDelegate() override;

  // Opera extensions
  OperaBrowserContext* browser_context();
  OperaBrowserContext* off_the_record_browser_context();
  OperaBrowserMainParts* opera_browser_main_parts() {
    return opera_browser_main_parts_;
  }

  // Initialize the flag settings, loading the configuration from storage
  // if necessary.
  void InitFlagPrefs();
  void InitPushMessagingStorage(const base::FilePath& dir);

  const scoped_refptr<FlagsJsonStorage>& flags_prefs() {
    return flags_prefs_;
  }

  PushMessagingJsonStorage& push_messaging_storage() {
    return *push_messaging_storage_;
  }

 private:
  OperaBrowserContext* OperaBrowserContextForBrowserContext(
      content::BrowserContext* content_browser_context);

  OperaBrowserMainParts* opera_browser_main_parts_;

  scoped_refptr<FlagsJsonStorage> flags_prefs_;
  std::unique_ptr<PushMessagingJsonStorage> push_messaging_storage_;
};

}  // namespace opera

#endif  // CHILL_BROWSER_OPERA_CONTENT_BROWSER_CLIENT_H_
