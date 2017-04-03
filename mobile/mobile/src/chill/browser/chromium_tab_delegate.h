// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_CHROMIUM_TAB_DELEGATE_H_
#define CHILL_BROWSER_CHROMIUM_TAB_DELEGATE_H_

#include <jni.h>

#include <string>
#include <vector>

#include "base/strings/string16.h"
#include "content/public/browser/certificate_request_result_type.h"
#include "third_party/WebKit/public/platform/WebReferrerPolicy.h"
#include "third_party/WebKit/public/web/WebAudioElementPlayState.h"

class GURL;

namespace autofill {
class PasswordForm;
}  // namespace autofill

namespace content {
enum class PermissionType;
class WebContents;
}  // namespace content

namespace opera {

class JavaScriptDialogManagerDelegate;
class ChromiumTab;

class PermissionDialogDelegate {
 public:
  virtual ~PermissionDialogDelegate() {}

  virtual void Allow(ChromiumTab* tab) {}
  virtual void Cancel(ChromiumTab* tab) {}
  virtual void Disallow(ChromiumTab* tab) {}
};

struct OpMultipleChoiceEntry {
  OpMultipleChoiceEntry(const std::string& id, const std::string& text)
      : id(id), text(text) {}
  OpMultipleChoiceEntry() {}
  std::string id;
  std::string text;
};

class MultipleChoiceDialogDelegate {
 public:
  explicit MultipleChoiceDialogDelegate(
      std::vector<OpMultipleChoiceEntry> values)
      : values_(values) {}
  virtual ~MultipleChoiceDialogDelegate() {}

  virtual void Selected(ChromiumTab* tab, const std::string& id) {}
  virtual void Cancel(ChromiumTab* tab) {}
  std::vector<OpMultipleChoiceEntry> values() { return values_; }

 private:
  std::vector<OpMultipleChoiceEntry> values_;
};

class ChromiumTabDelegate {
 public:
  virtual ~ChromiumTabDelegate() {}
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

  enum MultipleChoiceDialogType { AudioSource, VideoSource };

  virtual void RequestPermissionDialog(PermissionDialogType type,
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

  virtual void UpdatePlayState(blink::WebAudioElementPlayState play_state) = 0;

  // Utils
  static ChromiumTabDelegate* FromWebContents(
      const content::WebContents* web_contents);
  static ChromiumTabDelegate* FromID(int render_process_id, int render_view_id);
};

}  // namespace opera

#endif  // CHILL_BROWSER_CHROMIUM_TAB_DELEGATE_H_
