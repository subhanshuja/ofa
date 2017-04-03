// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from
//    chromium/src/content/shell/browser/shell_content_browser_client.cc
// @final-synchronized

#include "chill/browser/opera_content_browser_client.h"

#include "base/android/locale_utils.h"
#include "base/android/path_utils.h"
#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/files/file.h"
#include "base/path_service.h"
#include "components/crash/content/app/breakpad_linux.h"
#include "components/crash/content/browser/crash_handler_host_linux.h"
#include "components/crash/content/browser/crash_dump_manager_android.h"
#include "components/navigation_interception/intercept_navigation_delegate.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/browser_url_handler.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/resource_dispatcher_host.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "content/public/common/web_preferences.h"
#include "net/android/network_library.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/resource/resource_bundle_android.h"
#include "url/gurl.h"

#include "common/error_pages/ssl/ssl_blocking_page.h"
#include "common/settings/settings_manager.h"

#include "chill/app/native_interface.h"
#include "chill/browser/authentication_dialog.h"
#include "chill/browser/blocked_window_params.h"
#include "chill/browser/chromium_tab.h"
#include "chill/browser/chromium_tab_delegate.h"
#include "chill/browser/media_capture_devices_dispatcher.h"
#include "chill/browser/notifications/opera_platform_notification_service.h"
#include "chill/browser/opera_browser_context.h"
#include "chill/browser/opera_browser_main_parts.h"
#include "chill/browser/opera_devtools_manager_delegate.h"
#include "chill/browser/opera_quota_permission_context.h"
#include "chill/browser/opera_web_contents_view_delegate.h"
#include "chill/browser/popup_blocking_helper.h"
#include "chill/browser/service_tab_launcher.h"
#include "chill/common/opera_content_client.h"
#include "chill/common/opera_descriptors.h"
#include "chill/common/opera_switches.h"

using base::FilePath;
using base::CommandLine;
using content::BrowserThread;
using content::WebContents;
using navigation_interception::InterceptNavigationDelegate;

namespace opera {

namespace {

void OnCertificateResponseCallback(
    base::Callback<void(content::CertificateRequestResultType)> callback,
    ChromiumTabDelegate* delegate,
    content::CertificateRequestResultType result) {
  if (delegate) {
    delegate->OnCertificateErrorResponse(result);
  }
  callback.Run(result);
  callback.Reset();
}

bool FixupOperaURL(GURL* url, content::BrowserContext* browser_context) {
  if (!url->SchemeIs(content::kLegacyOperaUIScheme))
    return false;

  GURL::Replacements replacements;
  replacements.SetSchemeStr(content::kChromeUIScheme);
  *url = url->ReplaceComponents(replacements);
  return true;
}

}  // namespace

OperaContentBrowserClient* g_browser_client;

OperaContentBrowserClient* OperaContentBrowserClient::Get() {
  return g_browser_client;
}

OperaContentBrowserClient::OperaContentBrowserClient()
    : opera_browser_main_parts_(NULL) {
  DCHECK(!g_browser_client);
  g_browser_client = this;
}

OperaContentBrowserClient::~OperaContentBrowserClient() {
  g_browser_client = NULL;
}

content::BrowserMainParts* OperaContentBrowserClient::CreateBrowserMainParts(
    const content::MainFunctionParams& parameters) {
  opera_browser_main_parts_ = new OperaBrowserMainParts(parameters);
  return opera_browser_main_parts_;
}

void OperaContentBrowserClient::AppendExtraCommandLineSwitches(
    CommandLine* command_line, int child_process_id) {
  command_line->AppendSwitchASCII(
      switches::kUserAgent, GetUserAgent());

  if (CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnablePerformanceResourceTimingWasCached))
    command_line->AppendSwitch(
        switches::kEnablePerformanceResourceTimingWasCached);
  if (CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableCrashReporter)) {
    command_line->AppendSwitch(switches::kEnableCrashReporter);
  }
}

net::NetLog* OperaContentBrowserClient::GetNetLog() {
  return opera_browser_main_parts_->net_log();
}

content::QuotaPermissionContext*
OperaContentBrowserClient::CreateQuotaPermissionContext() {
  return new OperaQuotaPermissionContext();
}

void OperaContentBrowserClient::AllowCertificateError(
    content::WebContents* web_contents,
    int cert_error,
    const net::SSLInfo& ssl_info,
    const GURL& request_url,
    content::ResourceType resource_type,
    bool overridable,
    bool strict_enforcement,
    bool expired_previous_decision,
    const base::Callback<void(content::CertificateRequestResultType)>&
        callback) {
  if (resource_type != content::RESOURCE_TYPE_MAIN_FRAME) {
    // A sub-resource has a certificate error.  The user doesn't really
    // have a context for making the right decision, so block the
    // request hard, without an info bar to allow showing the insecure
    // content.
    if (!callback.is_null())
      callback.Run(content::CERTIFICATE_REQUEST_RESULT_TYPE_DENY);
    return;
  }

  ChromiumTab* tab = ChromiumTab::FromWebContents(web_contents);
  if (!tab) {
    if (!callback.is_null()) {
      callback.Run(content::CERTIFICATE_REQUEST_RESULT_TYPE_DENY);
    }
    NOTREACHED();
    return;
  }
  // Display an SSL blocking page.
  int options_mask = 0;
  if (overridable)
    options_mask |= SSLBlockingPage::OVERRIDABLE;
  if (strict_enforcement)
    options_mask |= SSLBlockingPage::STRICT_ENFORCEMENT;
  if (expired_previous_decision)
    options_mask |= SSLBlockingPage::EXPIRED_BUT_PREVIOUSLY_ALLOWED;
  SSLBlockingPage* ssl_blocking_page = new SSLBlockingPage(
      web_contents,
      cert_error,
      ssl_info,
      request_url,
      GetApplicationLocale(),
      options_mask,
      base::Bind(&OnCertificateResponseCallback, callback, tab->GetDelegate()));
  ssl_blocking_page->Show();
}

void OperaContentBrowserClient::AddCertificate(
    net::CertificateMimeType cert_type,
    const void* cert_data,
    size_t cert_size,
    int render_process_id,
    int render_frame_id) {
  if (cert_size > 0) {
    net::android::StoreCertificate(cert_type, cert_data, cert_size);
  }
}

void OperaContentBrowserClient::BrowserURLHandlerCreated(
    content::BrowserURLHandler* handler) {
  handler->SetFixupHandler(&FixupOperaURL);
}

void OperaContentBrowserClient::OpenURL(
    content::BrowserContext* browser_context,
    const content::OpenURLParams& params,
    const base::Callback<void(content::WebContents*)>& callback) {
  ServiceTabLauncher::GetInstance()->LaunchTab(browser_context, params,
                                               callback);
}

ScopedVector<content::NavigationThrottle>
OperaContentBrowserClient::CreateThrottlesForNavigation(
    content::NavigationHandle* navigation_handle) {
  ScopedVector<content::NavigationThrottle> throttles;
  if (navigation_handle->IsInMainFrame()) {
    throttles.push_back(
        InterceptNavigationDelegate::CreateThrottleFor(navigation_handle));
  }
  return throttles;
}

void OperaContentBrowserClient::OverrideWebkitPrefs(
    content::RenderViewHost* render_view_host,
    content::WebPreferences* prefs) {
  prefs->javascript_enabled = SettingsManager::GetJavaScript();

  prefs->text_wrapping_enabled = SettingsManager::GetTextWrap();

  prefs->text_autosizing_enabled = !SettingsManager::GetTextWrap();

  prefs->force_enable_zoom = SettingsManager::GetForceEnableZoom();

  prefs->password_echo_enabled = true;

  prefs->video_compression_enabled = SettingsManager::GetCompression();

  // WAM-8226: Let Chrome take the fight and remove this after the dust settles.
  prefs->allow_geolocation_on_insecure_origins = true;

  content::WebContents* web_contents =
      content::WebContents::FromRenderViewHost(render_view_host);
  if (web_contents) {
    ChromiumTab* tab = ChromiumTab::FromWebContents(web_contents);
    if (tab)
      tab->OverrideWebkitPrefs(prefs);
  }
}

void HandlePopupBlockingOnUIThread(const BlockedWindowParams& params) {
  content::RenderFrameHost* render_frame_host =
        content::RenderFrameHost::FromID(params.render_process_id(),
                                         params.opener_render_frame_id());
  if (!render_frame_host) return;

  WebContents* web_contents =
    WebContents::FromRenderFrameHost(render_frame_host);
  if (!web_contents ||
      (!render_frame_host->GetParent() &&
       web_contents->GetMainFrame() != render_frame_host))
      return;

  PopupBlockingHelper* popup_helper =
    PopupBlockingHelper::FromWebContents(web_contents);
  if (!popup_helper) return;

  popup_helper->HandleBlockedPopup(params);
}

content::PlatformNotificationService*
      OperaContentBrowserClient::GetPlatformNotificationService() {
  return OperaPlatformNotificationService::GetInstance();
}

bool OperaContentBrowserClient::CanCreateWindow(
    const GURL& opener_url,
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
    bool* no_javascript_access) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (no_javascript_access) {
    *no_javascript_access = false;
  }

  if (!user_gesture &&
       SettingsManager::GetBlockPopups() == SettingsManager::ON) {
    BlockedWindowParams blocked_params(target_url,
                                       render_process_id,
                                       opener_render_frame_id);
    BrowserThread::PostTask(BrowserThread::UI,
                            FROM_HERE,
                            base::Bind(&HandlePopupBlockingOnUIThread,
                                       blocked_params));

    return false;
  }
  return true;
}

void OperaContentBrowserClient::ResourceDispatcherHostCreated() {
  // Delegate created in
  // com.opera.android.browser.BrowserManager
}

std::string OperaContentBrowserClient::GetDefaultDownloadName() {
  return "download";
}

FilePath OperaContentBrowserClient::GetDefaultDownloadDirectory() {
  FilePath downloads_dir;
  base::android::GetDownloadsDirectory(&downloads_dir);
  return downloads_dir;
}

content::WebContentsViewDelegate*
OperaContentBrowserClient::GetWebContentsViewDelegate(
    content::WebContents* web_contents) {
  return CreateOperaWebContentsViewDelegate(web_contents);
}

void OperaContentBrowserClient::GetAdditionalMappedFilesForChildProcess(
    const CommandLine& command_line,
    int child_process_id,
    content::FileDescriptorInfo* mappings,
    std::map<int, base::MemoryMappedFile::Region>* regions) {
  int fd = ui::GetMainAndroidPackFd(
      &(*regions)[kOperaPakDescriptor]);
  mappings->Share(kOperaPakDescriptor, fd);

  fd = ui::GetLocalePackFd(&(*regions)[kLocalePakDescriptor]);
  mappings->Share(kLocalePakDescriptor, fd);

  if (breakpad::IsCrashReporterEnabled()) {
    base::File f(breakpad::CrashDumpManager::GetInstance()->CreateMinidumpFile(
        child_process_id));
    if (!f.IsValid()) {
      LOG(ERROR) << "Failed to create file for minidump, crash reporting will "
                 << "be disabled for this process.";
    } else {
      mappings->Transfer(kMinidumpDescriptor,
                         base::ScopedFD(f.TakePlatformFile()));
    }
  }
}

OperaBrowserContext* OperaContentBrowserClient::browser_context() {
  return opera_browser_main_parts_->browser_context();
}

OperaBrowserContext*
OperaContentBrowserClient::off_the_record_browser_context() {
  return opera_browser_main_parts_->off_the_record_browser_context();
}


content::MediaObserver* OperaContentBrowserClient::GetMediaObserver() {
  return MediaCaptureDevicesDispatcher::GetInstance();
}

OperaBrowserContext*
OperaContentBrowserClient::OperaBrowserContextForBrowserContext(
    content::BrowserContext* content_browser_context) {
  if (content_browser_context == browser_context())
    return browser_context();
  DCHECK_EQ(content_browser_context, off_the_record_browser_context());
  return off_the_record_browser_context();
}

void OperaContentBrowserClient::InitFlagPrefs() {
  DCHECK(!flags_prefs_.get());

  // Invoking the constructor will immediately load the flag configuration
  // from the JSON storage.
  flags_prefs_ = new FlagsJsonStorage();
}

void OperaContentBrowserClient::InitPushMessagingStorage(
    const base::FilePath& dir) {
  DCHECK(!push_messaging_storage_.get());
  push_messaging_storage_.reset(new PushMessagingJsonStorage(dir));
}

std::string OperaContentBrowserClient::GetApplicationLocale() {
  return base::android::GetDefaultLocale();
}

content::DevToolsManagerDelegate*
OperaContentBrowserClient::GetDevToolsManagerDelegate() {
  return new OperaDevToolsManagerDelegate(browser_context());
}

}  // namespace opera
