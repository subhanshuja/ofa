// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/chromium_tab.h"

#include <utility>

#include <android/bitmap.h>

#include "base/android/jni_android.h"
#include "base/callback_helpers.h"
#include "base/memory/ptr_util.h"
#include "cc/layers/layer.h"
#include "components/navigation_interception/intercept_navigation_delegate.h"
#include "components/navigation_interception/navigation_params.h"
#include "content/browser/gpu/compositor_util.h"
#include "content/browser/renderer_host/render_widget_host_view_android.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/common/view_messages.h"
#include "content/public/browser/android/content_view_core.h"
#include "content/public/browser/android/content_view_layer_renderer.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/ssl_status.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/renderer_preferences.h"
#include "content/public/common/result_codes.h"
#include "content/public/common/security_style.h"
#include "content/public/common/web_preferences.h"
#include "net/http/http_stream_factory.h"
#include "net/turbo/turbo_constants.h"
#include "net/turbo/turbo_session_pool.h"
#include "net/url_request/url_request_context_getter.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/page_transition_types.h"
#include "ui/gfx/android/device_display_info.h"
#include "ui/gfx/android/java_bitmap.h"
#include "ui/gfx/geometry/point_conversions.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/transform.h"
#include "ui/surface/transport_dib.h"

#include "common/fraud_protection_ui/fraud_protection.h"
#include "common/settings/settings_manager.h"

#include "chill/browser/authentication_dialog.h"
#include "chill/browser/chromium_tab_delegate.h"
#include "chill/browser/history_tab_helper.h"
#include "chill/browser/navigation_history.h"
#include "chill/browser/net/cookie_policy.h"
#include "chill/browser/opera_browser_context.h"
#include "chill/browser/opera_content_browser_client.h"
#include "chill/browser/opera_fraud_protection_delegate.h"
#include "chill/browser/opera_fraud_protection_service.h"
#include "chill/browser/opera_javascript_dialog_manager.h"
#include "chill/browser/ui/tabs/bitmap_sink.h"
#include "chill/common/opera_content_client.h"

namespace opera {

using content::WebContents;
using navigation_interception::InterceptNavigationDelegate;

namespace {

class BitmapRequest {
  class ReadbackCallback {
   public:
    explicit ReadbackCallback(const tabui::BitmapSink& sink) : sink_(sink) {}

    void Run(const SkBitmap& bitmap, content::ReadbackResponse response) {
      if (response == content::READBACK_SUCCESS) {
        sink_.Accept(bitmap);
      } else {
        sink_.Fail();
      }
    }

   private:
    tabui::BitmapSink sink_;
  };

 public:
  BitmapRequest(content::RenderWidgetHostViewAndroid* rwhva,
                const tabui::BitmapSink& sink)
      : rwhva_(rwhva), sink_(sink) {}

  void Run() {
    ReadbackCallback* callback = new ReadbackCallback(sink_);
    rwhva_->GetScaledContentBitmap(
        1, SkColorType::kRGBA_8888_SkColorType, gfx::Rect(),
        base::Bind(&ReadbackCallback::Run, base::Owned(callback)));
  }

 private:
  content::RenderWidgetHostViewAndroid* rwhva_;
  tabui::BitmapSink sink_;
};

void SetTurboEnabled(bool turbo_enabled,
                     bool turbo_video_enabled,
                     bool turbo_ad_blocking_enabled) {
  net::HttpStreamFactory::set_turbo2_enabled(turbo_enabled);
  net::TurboSessionPool::SetAdblockBlankEnabled(
      turbo_ad_blocking_enabled);
  net::TurboSessionPool::SetVideoCompressionEnabled(
      turbo_video_enabled);
}

void SetTurboImageQuality(net::TurboImageQuality image_mode) {
  net::TurboSessionPool::SetImageQuality(image_mode);
}

}  // namespace

const char kTabUserDataKey[] = "TabUserData";

class TabUserData : public base::SupportsUserData::Data {
 public:
  explicit TabUserData(ChromiumTab* tab) : tab_(tab) {}
  virtual ~TabUserData() {}
  ChromiumTab* tab() { return tab_; }

 private:
  ChromiumTab* tab_;  // unowned
};

ChromiumTab::ChromiumTab(bool is_private, bool is_webapp)
    : is_private_(is_private),
      is_webapp_(is_webapp),
      registrar_(new content::NotificationRegistrar),
      auth_dialog_delegate_(NULL),
      delegate_(NULL) {}

ChromiumTab::~ChromiumTab() {}

void ChromiumTab::Initialize(jobject new_web_contents) {
  content::BrowserContext* browser_context =
      is_private_ ? OperaBrowserContext::GetPrivateBrowserContext()
                  : OperaBrowserContext::GetDefaultBrowserContext();
  DCHECK(browser_context);

  if (new_web_contents) {
    web_contents_.reset(WebContents::FromJavaWebContents(
        base::android::ScopedJavaLocalRef<jobject>(
            base::android::AttachCurrentThread(), new_web_contents)));
  } else {
    // TODO(unknown): pass CreateParams into ChromiumTab
    WebContents::CreateParams params(browser_context, NULL);
    params.initially_hidden = true;
    params.routing_id = MSG_ROUTING_NONE;
    web_contents_.reset(WebContents::Create(params));
  }

  web_contents_->GetMutableRendererPrefs()->use_subpixel_positioning = true;
  web_contents_->GetMutableRendererPrefs()->tap_multiple_targets_strategy =
      content::TAP_MULTIPLE_TARGETS_STRATEGY_ZOOM;

  // Stash this in the WebContents.
  web_contents_.get()->SetUserData(&kTabUserDataKey, new TabUserData(this));

  const content::NavigationController& controller =
      web_contents_->GetController();

  registrar_->Add(this, content::NOTIFICATION_NAV_ENTRY_CHANGED,
                  content::Source<content::NavigationController>(&controller));

  registrar_->Add(this, content::NOTIFICATION_NAV_LIST_PRUNED,
                  content::Source<content::NavigationController>(&controller));

  password_form_manager_.reset(new PasswordFormManager(this));

  content::StoragePartition* partition =
      content::BrowserContext::GetStoragePartition(browser_context, NULL);

  fraud_protection_service_.reset(
      new OperaFraudProtectionService(partition->GetURLRequestContext()));

  FraudProtection::CreateForWebContents(web_contents_.get());

  if (!is_private_) {
    HistoryTabHelper::CreateForWebContents(web_contents_.get());
  }

  web_contents_observer_.reset(
      new OperaWebContentsObserver(web_contents_.get()));
}

void ChromiumTab::Destroy() {
  pending_frame_callbacks_.clear();
  registrar_.reset();
  web_contents_->SetDelegate(nullptr);
  web_contents_delegate_.reset();
  content::RenderProcessHost* process = web_contents_->GetRenderProcessHost();
  if (process) {
    process->FastShutdownForPageCount(1);
  }
  web_contents_.reset();
  java_script_dialog_manager_.reset();
  web_contents_observer_.reset();
  password_form_manager_.reset();
  fraud_protection_service_.reset();
  auth_dialog_delegate_ = nullptr;
  delegate_ = nullptr;
}

void ChromiumTab::SetInterceptNavigationDelegate(jobject delegate) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  JNIEnv* env = base::android::AttachCurrentThread();
  InterceptNavigationDelegate::Associate(
      web_contents_.get(),
      base::WrapUnique(new InterceptNavigationDelegate(env, delegate)));
}

void ChromiumTab::RequestUpdateWebkitPreferences() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
  // OnWebkitPreferencesChanged() will trigger a call to
  // ContentBrowserClient::OverrideWebkitPrefs(), which in turn queries the
  // Opera preferences system (on the Java side)
  web_contents_->GetRenderViewHost()->OnWebkitPreferencesChanged();

  content::BrowserContext* browser_context = web_contents_->GetBrowserContext();
  net::URLRequestContextGetter* url_request_context =
      content::BrowserContext::GetDefaultStoragePartition(browser_context)->GetURLRequestContext();

  CookiePolicy::GetInstance()->UpdateFromSettings();

  bool turbo_enabled = SettingsManager::GetCompression();
  bool turbo_video_enabled = SettingsManager::GetVideoCompression();
  bool turbo_ad_blocking_enabled = SettingsManager::GetAdBlocking();
  url_request_context->GetNetworkTaskRunner()->PostTask(
      FROM_HERE, base::Bind(SetTurboEnabled, turbo_enabled, turbo_video_enabled,
                            turbo_ad_blocking_enabled));

  net::TurboImageQuality image_mode =
      turbo_enabled ? SettingsManager::GetTurboImageQualityMode()
                    : net::TurboImageQuality::LOSSLESS;
  url_request_context->GetNetworkTaskRunner()->PostTask(
      FROM_HERE, base::Bind(SetTurboImageQuality, image_mode));

  // Signal all RenderProcessHosts to discard images of lower quality than
  // `image_mode'.
  content::RenderProcessHost* rph = web_contents_->GetRenderProcessHost();
  DCHECK(rph != NULL);
  rph->SignalAllTurboImageQuality(image_mode);
}

void ChromiumTab::OverrideWebkitPrefs(content::WebPreferences* prefs) {
  if (IsWebApp()) {
    prefs->text_wrapping_enabled = false;
    prefs->text_autosizing_enabled = true;
    prefs->force_enable_zoom = false;
  }
}

void ChromiumTab::EnableEncodingAutodetect(bool enable) {
  // Can not use RenderViewHost.UpdateWebkitPreferences directly since prefs
  // will be reset on next page load.
  is_encoding_autodetect_enabled_ = enable;
}

void ChromiumTab::Focus() {
  delegate_->Focus();
}

void ChromiumTab::RequestBitmap(tabui::BitmapSink* sink) {
  if (!web_contents_) {
    sink->Fail();
    return;
  }

  content::RenderWidgetHostViewAndroid* rwhva =
      static_cast<content::RenderWidgetHostViewAndroid*>(
          web_contents_->GetRenderWidgetHostView());
  if (!rwhva) {
    sink->Fail();
    return;
  }

  std::unique_ptr<BitmapRequest> request(new BitmapRequest(rwhva, *sink));

  // Submit request immediately if surface is available.
  if (rwhva->IsSurfaceAvailableForCopy()) {
    request->Run();
    return;
  }

  sink->Fail();
}

void ChromiumTab::Cut() {
  web_contents_->Cut();
}

void ChromiumTab::Paste() {
  web_contents_->Paste();
}

void ChromiumTab::RequestFrameCallback(OpRunnable callback) {
  base::Closure frame_callback = base::Bind(callback, OpArguments());
  content::RenderWidgetHostView* rwhv =
      web_contents_->GetRenderWidgetHostView();
  if (rwhv) {
    rwhv->GetRenderWidgetHost()->RequestFrameCallback(frame_callback);
  } else {
    bool registered = registrar_->IsRegistered(
        this, content::NOTIFICATION_WEB_CONTENTS_RENDER_VIEW_HOST_CREATED,
        content::Source<WebContents>(web_contents_.get()));
    if (!registered) {
      registrar_->Add(
          this, content::NOTIFICATION_WEB_CONTENTS_RENDER_VIEW_HOST_CREATED,
          content::Source<WebContents>(web_contents_.get()));
    }

    pending_frame_callbacks_.push_back(frame_callback);
  }
}

void ChromiumTab::RequestPendingFrameCallbacks(content::RenderViewHost* host) {
  for (std::vector<base::Closure>::iterator it =
           pending_frame_callbacks_.begin();
       it != pending_frame_callbacks_.end(); it++) {
    host->GetWidget()->RequestFrameCallback(*it);
  }
  pending_frame_callbacks_.clear();
}

void ChromiumTab::RestartHangMonitorTimeout() {
  web_contents_->GetRenderViewHost()->GetWidget()->RestartHangMonitorTimeout();
}

void ChromiumTab::OnAuthenticationDialogRequest(
    AuthenticationDialogDelegate* auth) {
  DCHECK(!auth_dialog_delegate_);
  auth_dialog_delegate_ = auth;
  if (delegate_->IsActive()) {
    ShowAuthenticationDialogIfNecessary();
  }
}

void ChromiumTab::ClearAuthenticationDialogRequest() {
  auth_dialog_delegate_ = NULL;
}

void ChromiumTab::ShowAuthenticationDialogIfNecessary() {
  if (auth_dialog_delegate_) {
    auth_dialog_delegate_->OnShow();
    auth_dialog_delegate_ = NULL;
  }
}

void ChromiumTab::KillProcess(bool wait) {
  content::RenderProcessHost* rph = web_contents_->GetRenderProcessHost();
  if (rph) {
    rph->Shutdown(content::RESULT_CODE_HUNG, wait);
  }
}

ChromiumTabDelegate* ChromiumTab::GetDelegate() const {
  return delegate_;
}

void ChromiumTab::SetWebContentsDelegate(jobject jweb_contents_delegate) {
  std::unique_ptr<OperaWebContentsDelegate> web_contents_delegate;
  if (jweb_contents_delegate) {
    JNIEnv* env = base::android::AttachCurrentThread();
    web_contents_delegate.reset(
        new OperaWebContentsDelegate(env, jweb_contents_delegate));
  }
  web_contents_delegate_.reset(web_contents_delegate.release());
  web_contents_->SetDelegate(web_contents_delegate_.get());
}

void ChromiumTab::SetDelegate(ChromiumTabDelegate* delegate) {
  delegate_ = delegate;
}

void ChromiumTab::SetPasswordManagerDelegate(
    PasswordManagerDelegate* delegate) {
  password_form_manager_->SetDelegate(delegate);
}

WebContents* ChromiumTab::GetWebContents() {
  return web_contents_.get();
}

jobject ChromiumTab::GetJavaWebContents() {
  return web_contents_->GetJavaWebContents().Release();
}

ChromiumTab* ChromiumTab::FromWebContents(const WebContents* web_contents) {
  // Not all WebContents have a ChromiumTab connected to it. The resume
  // interrupted downloads (read from disk) WebContents, for example, doesn't
  // have one.
  TabUserData* data =
      static_cast<TabUserData*>(web_contents->GetUserData(&kTabUserDataKey));
  if (data) {
    return data->tab();
  }
  return NULL;
}

ChromiumTab* ChromiumTab::FromID(int render_process_id, int render_frame_id) {
  content::RenderFrameHost* render_frame_host =
      content::RenderFrameHost::FromID(render_process_id, render_frame_id);

  if (!render_frame_host)
    return NULL;

  WebContents* web_contents =
      WebContents::FromRenderFrameHost(render_frame_host);

  if (!web_contents)
    return NULL;

  return ChromiumTab::FromWebContents(web_contents);
}

void ChromiumTab::Observe(int type,
                          const content::NotificationSource& source,
                          const content::NotificationDetails& details) {
  switch (type) {
    case content::NOTIFICATION_NAV_LIST_PRUNED: {
      const content::PrunedDetails* pd =
          content::Details<content::PrunedDetails>(details).ptr();
      delegate_->NavigationHistoryPruned(pd->from_front, pd->count);
      break;
    }
    case content::NOTIFICATION_NAV_ENTRY_CHANGED: {
      content::Details<content::EntryChangedDetails> changed(details);
      const content::NavigationController& controller =
          web_contents_->GetController();
      if (controller.GetActiveEntry() == changed->changed_entry)
        delegate_->ActiveNavigationEntryChanged();
      break;
    }
    case content::NOTIFICATION_WEB_CONTENTS_RENDER_VIEW_HOST_CREATED: {
      content::RenderViewHost* host =
          content::Details<content::RenderViewHost>(details).ptr();
      RequestPendingFrameCallbacks(host);
      registrar_->Remove(
          this, content::NOTIFICATION_WEB_CONTENTS_RENDER_VIEW_HOST_CREATED,
          content::Source<WebContents>(web_contents_.get()));
      break;
    }
    default:
      NOTREACHED() << "Unexpected notification type: " << type;
  }
}


FraudProtectionService* ChromiumTab::GetFraudProtectionService() const {
  return fraud_protection_service_.get();
}

OperaWebContentsObserver* ChromiumTab::web_contents_observer() {
  return web_contents_observer_.get();
}

bool ChromiumTab::IsPasswordFormManagerWaitingForDidFinishLoad() const {
  return password_form_manager_->IsWaitingForDidFinishLoad();
}

void ChromiumTab::Navigated(const WebContents* web_contents) {
  CHECK(delegate_);
  content::NavigationEntry* entry =
      web_contents->GetController().GetActiveEntry();
  if (entry == NULL) {
    return;
  }

  ui::PageTransition transition_type = entry->GetTransitionType();

  delegate_->Navigated(entry->GetUniqueID(), entry->GetOriginalRequestURL(),
                       entry->GetVirtualURL(),
                       ui::PageTransitionStripQualifier(transition_type) ==
                               ui::PAGE_TRANSITION_TYPED
                           ? entry->GetUserTypedURL()
                           : GURL::EmptyGURL(),
                       entry->GetTitle(),
                       entry->GetPageType() == content::PAGE_TYPE_INTERSTITIAL,
                       transition_type);
}

ChromiumTab::SecurityLevel ChromiumTab::GetSecurityLevel() {
  content::NavigationEntry* entry =
      web_contents_->GetController().GetVisibleEntry();
  if (entry &&
      entry->GetSSL().content_status == content::SSLStatus::NORMAL_CONTENT) {
    switch (entry->GetSSL().security_style) {
      case content::SECURITY_STYLE_UNKNOWN:
      case content::SECURITY_STYLE_UNAUTHENTICATED:
      case content::SECURITY_STYLE_AUTHENTICATION_BROKEN:
        return INSECURE;
      case content::SECURITY_STYLE_AUTHENTICATED:
        return SECURE;
      default:
        DCHECK(false && "SecurityStyle changed");
        return INSECURE;
    }
  } else {
    return INSECURE;
  }
}

std::string ChromiumTab::GetOriginalRequestURL() {
  content::NavigationEntry* entry =
      web_contents_->GetController().GetVisibleEntry();
  // We might get NavigationStateChanged after discarding a
  // transient entry, and then the list of entries can be
  // null. In that case we need to show a placeholder URL to
  // avoid address bar spoofing, see WAM-4108.
  if (entry) {
    return entry->GetOriginalRequestURL().spec();
  } else {
    return std::string();
  }
}

OperaJavaScriptDialogManager* ChromiumTab::GetJavaScriptDialogManager() {
  if (!java_script_dialog_manager_) {
    java_script_dialog_manager_.reset(new OperaJavaScriptDialogManager(
        delegate_->GetJavaScriptDialogManagerDelegate()));
  }
  return java_script_dialog_manager_.get();
}

bool ChromiumTab::IsPrivateTab() const {
  return is_private_;
}

bool ChromiumTab::IsWebApp() const {
  return is_webapp_;
}

}  // namespace opera
