// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include <jni.h>
#include "chill/browser/chromium_tab.h"
%}

// Same as SpecialRoutingIDs in <ipc/ipc_message.h>
enum SpecialRoutingIDs {
  MSG_ROUTING_NONE = -2,
  MSG_ROUTING_CONTROL = 2147483647, // kint32max
};


namespace opera {

%nodefaultctor ChromiumTab;
%rename(NativeChromiumTab) ChromiumTab;

// Apply mixedCase to method names in Java.
%rename("%(regex:/ChromiumTab::([A-Z].*)/\\l\\1/)s", regextarget=1, fullname=1, %$isfunction) ".*";

// Protected methods

%javamethodmodifiers ChromiumTab::ChromiumTab "protected";
%javamethodmodifiers ChromiumTab::Initialize "protected";
%javamethodmodifiers ChromiumTab::Destroy "protected";
%javamethodmodifiers ChromiumTab::GetSecurityLevel "protected";
%javamethodmodifiers ChromiumTab::GetJavaWebContents "protected";
%javamethodmodifiers ChromiumTab::SetDelegate "protected";
%javamethodmodifiers ChromiumTab::SetIsInBackground "protected";
%javamethodmodifiers ChromiumTab::SetPasswordManagerDelegate "protected";
%javamethodmodifiers ChromiumTab::SetWebContentsDelegate "protected";
%javamethodmodifiers ChromiumTab::RequestFrameCallback "protected";

class ChromiumTab {
public:
  enum SecurityLevel {
    SECURE,
    INSECURE,
  };

  ChromiumTab(bool is_private, bool is_webapp);

  virtual ~ChromiumTab();

  void Initialize(jobject new_web_contents);

  void Destroy();

  void SetInterceptNavigationDelegate(jobject delegate);

  void SetWebContentsDelegate(jobject jweb_contents_delegate);

  void SetDelegate(ChromiumTabDelegate *delegate);

  void SetPasswordManagerDelegate(PasswordManagerDelegate* delegate);

  jobject GetJavaWebContents();

  void RequestUpdateWebkitPreferences();

  void EnableEncodingAutodetect(bool enable);

  void ShowAuthenticationDialogIfNecessary();

  void ClearAuthenticationDialogRequest();

  void RequestBitmap(opera::tabui::BitmapSink* sink);

  void Paste();

  void RequestFrameCallback(OpRunnable callback);

  void RestartHangMonitorTimeout();

  void KillProcess(bool wait);

  bool IsPasswordFormManagerWaitingForDidFinishLoad() const;

  SecurityLevel GetSecurityLevel();

  std::string GetOriginalRequestURL();

  bool IsPrivateTab() const;

  bool IsWebApp() const;
};

}  // namespace opera
