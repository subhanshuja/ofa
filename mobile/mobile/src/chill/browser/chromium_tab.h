// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_CHROMIUM_TAB_H_
#define CHILL_BROWSER_CHROMIUM_TAB_H_

#include <jni.h>

#include <map>
#include <string>
#include <vector>

#include "base/android/jni_android.h"
#include "base/callback.h"
#include "base/memory/scoped_vector.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "ipc/ipc_channel.h"

#include "common/fraud_protection/fraud_protection_service.h"

#include "chill/browser/opera_web_contents_delegate.h"
#include "chill/browser/opera_web_contents_observer.h"
#include "chill/browser/password_manager/password_form_manager.h"
#include "common/swig_utils/op_arguments.h"
#include "common/swig_utils/op_runnable.h"

namespace content {
class RenderViewHost;
class RenderWidgetHostViewAndroid;
class ResourceThrottle;
struct WebPreferences;
}

namespace gfx {
class Point;
}

namespace net {
class URLRequest;
}

namespace opera {

class AuthenticationDialogDelegate;
class NavigationHistory;
class OperaJavaScriptDialogManager;
class PasswordManagerDelegate;
class ChromiumTabDelegate;

namespace tabui {
class BitmapSink;
}

class ChromiumTab : public content::NotificationObserver {
 public:
  enum SecurityLevel {
    SECURE,
    INSECURE,
  };

  explicit ChromiumTab(bool is_private, bool is_webapp);
  virtual ~ChromiumTab();

  void Initialize(jobject new_web_contents);

  void Destroy();

  void SetInterceptNavigationDelegate(jobject delegate);
  void SetWebContentsDelegate(jobject jweb_contents_delegate);
  ChromiumTabDelegate* GetDelegate() const;
  virtual void SetDelegate(ChromiumTabDelegate* delegate);
  virtual content::WebContents* GetWebContents();
  jobject GetJavaWebContents();

  void SetPasswordManagerDelegate(PasswordManagerDelegate* delegate);

  static ChromiumTab* FromWebContents(const content::WebContents* web_contents);

  static ChromiumTab* FromID(int render_process_id, int render_frame_id);

  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details);

  // This method should be called from Java whenever a WebKit setting (e.g.
  // enable/disable JavaScript) has changed on the Java side that needs to be
  // pushed down to WebKit.
  void RequestUpdateWebkitPreferences();

  void OverrideWebkitPrefs(content::WebPreferences* prefs);

  void EnableEncodingAutodetect(bool enable);

  void Cut();

  void Paste();

  void Focus();

  // Requests a new bitmap to be generated. Calls the sink's Accept() method in
  // case of success or the sink's Fail() method in case of failure.
  void RequestBitmap(tabui::BitmapSink* sink);

  void RequestFrameCallback(OpRunnable callback);

  void RestartHangMonitorTimeout();

  void OnAuthenticationDialogRequest(AuthenticationDialogDelegate* auth);

  void ClearAuthenticationDialogRequest();

  void ShowAuthenticationDialogIfNecessary();

  void KillProcess(bool wait);

  FraudProtectionService* GetFraudProtectionService() const;

  OperaWebContentsObserver* web_contents_observer();

  bool IsPasswordFormManagerWaitingForDidFinishLoad() const;

  void Navigated(const content::WebContents* web_contents);

  SecurityLevel GetSecurityLevel();

  // returns the url originally used to request the current page
  std::string GetOriginalRequestURL();

  OperaJavaScriptDialogManager* GetJavaScriptDialogManager();

  // Return whether the tab is a private tab or not.
  bool IsPrivateTab() const;

  bool IsWebApp() const;

 private:
  void RequestPendingFrameCallbacks(content::RenderViewHost* host);

  // Whether the tab is a private tab or not.
  bool is_private_;

  bool is_webapp_;

  bool is_encoding_autodetect_enabled_;

  std::vector<base::Closure> pending_frame_callbacks_;

  // Notification manager
  std::unique_ptr<content::NotificationRegistrar> registrar_;

  // The WebContents destructor calls OperaJavaScriptDialogManager so
  // this member must be first!
  std::unique_ptr<OperaJavaScriptDialogManager> java_script_dialog_manager_;
  std::unique_ptr<content::WebContents> web_contents_;
  std::unique_ptr<OperaWebContentsDelegate> web_contents_delegate_;
  std::unique_ptr<OperaWebContentsObserver> web_contents_observer_;
  std::unique_ptr<PasswordFormManager> password_form_manager_;
  std::unique_ptr<FraudProtectionService> fraud_protection_service_;

  AuthenticationDialogDelegate* auth_dialog_delegate_;
  ChromiumTabDelegate* delegate_;
};

}  // namespace opera

#endif  // CHILL_BROWSER_CHROMIUM_TAB_H_
