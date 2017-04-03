// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/chromium_tab_delegate.h"
%}

// Generate enum for blink::WebReferrerPolicy
%include <third_party/WebKit/public/platform/WebReferrerPolicy.h>

// Generate enum for blink::WebAudioElementPlayState
%include <third_party/WebKit/public/web/WebAudioElementPlayState.h>

// Generate enum for content::CertificateRequestResultType
%include <content/public/browser/certificate_request_result_type.h>

VECTOR(OpMultipleChoiceVector, opera::OpMultipleChoiceEntry);

namespace opera {

class PermissionDialogDelegate {
 public:
  virtual ~PermissionDialogDelegate() {}

  virtual void Allow(opera::ChromiumTab* tab) {}
  virtual void Cancel(opera::ChromiumTab* tab) {}
  virtual void Disallow(opera::ChromiumTab* tab) {}
};

struct OpMultipleChoiceEntry {
  std::string id;
  std::string text;
};

class MultipleChoiceDialogDelegate {
 public:
  virtual ~MultipleChoiceDialogDelegate();

  virtual void Selected(opera::ChromiumTab* tab, const std::string& id);
  virtual void Cancel(opera::ChromiumTab* tab);
  std::vector<OpMultipleChoiceEntry> values();
 private:
  MultipleChoiceDialogDelegate(std::vector<OpMultipleChoiceEntry> values);
};

%feature("director", assumeoverride=1) ChromiumTabDelegate;
%rename ("NativeChromiumTabDelegate") ChromiumTabDelegate;

// Suppress "Returning a pointer or reference in a director method is not recommended."
%warnfilter(473) ChromiumTabDelegate;

class JavaScriptDialogManagerDelegate;

class ChromiumTabDelegate {
public:
  ChromiumTabDelegate();
  virtual ~ChromiumTabDelegate();

  virtual jobject GetChromiumTab() = 0;
  virtual void Navigated(int navigation_entry_id,
                         const GURL& request_url,
                         const GURL& url,
                         const GURL& user_typed_url,
                         const base::string16& title,
                         bool internal_page,
                         int transition_type) = 0;
  virtual void ActiveNavigationEntryChanged() = 0;
  virtual void NavigationHistoryPruned(bool from_front, int count) = 0;
  virtual void Focus() = 0;
  virtual bool IsActive() = 0;

  virtual void OnCertificateErrorResponse(
      content::CertificateRequestResultType result) = 0;

  virtual void MediaStreamState(bool recording) = 0;

  enum PermissionDialogType {
    QuotaPermission,
  };

  enum MultipleChoiceDialogType {
    AudioSource,
    VideoSource
  };

  virtual void RequestPermissionDialog(
      PermissionDialogType type,
      const std::string&,
      PermissionDialogDelegate* delegate) = 0;
  virtual void RequestMultipleChoiceDialog(
      MultipleChoiceDialogType type,
      const std::string&,
      MultipleChoiceDialogDelegate* delegate) = 0;

  virtual int GetDisplayMode() = 0;

  virtual void FindReply(int request_id, int current, int max) = 0;
  virtual JavaScriptDialogManagerDelegate*
  GetJavaScriptDialogManagerDelegate() = 0;
  virtual void UpdatePlayState(
      blink::WebAudioElementPlayState play_state) = 0;
};

}  // namespace opera
