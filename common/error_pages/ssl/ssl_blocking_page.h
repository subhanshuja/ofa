// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/browser/ssl/ssl_blocking_page.h
// @final-synchronized

#ifndef COMMON_ERROR_PAGES_SSL_SSL_BLOCKING_PAGE_H_
#define COMMON_ERROR_PAGES_SSL_SSL_BLOCKING_PAGE_H_

#include <string>

#include "base/callback.h"
#include "content/public/browser/interstitial_page_delegate.h"
#include "content/public/browser/certificate_request_result_type.h"
#include "net/ssl/ssl_info.h"
#include "url/gurl.h"

namespace content {
class InterstitialPage;
class WebContents;
}

// This class is responsible for showing/hiding the interstitial page that is
// shown when a certificate error happens.
// It deletes itself when the interstitial page is closed.
class SSLBlockingPage : public content::InterstitialPageDelegate {
 public:
  // These represent the commands sent from the interstitial JavaScript. They
  // are defined in common/resources/webui/ssl_roadblock.js.
  // DO NOT reorder or change these without also changing the JavaScript!
  enum SSLBlockingPageCommands {
    CMD_DONT_PROCEED = 0,
    CMD_PROCEED = 1,
    CMD_RELOAD = 2
  };

  enum SSLBlockingPageOptionsMask {
    OVERRIDABLE = 1 << 0,
    STRICT_ENFORCEMENT = 1 << 1,
    EXPIRED_BUT_PREVIOUSLY_ALLOWED = 1 << 2
  };

  ~SSLBlockingPage() override;

  // Create an interstitial and show it.
  void Show();

  // Creates an SSL blocking page. If the blocking page isn't shown, the caller
  // is responsible for cleaning up the blocking page, otherwise the
  // interstitial takes ownership when shown.
  SSLBlockingPage(
      content::WebContents* web_contents,
      int cert_error,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      const std::string& app_locale,
      int options_mask,
      const base::Callback<
          void(content::CertificateRequestResultType)>& callback);

 protected:
  // InterstitialPageDelegate implementation.
  std::string GetHTMLContents() override;
  void CommandReceived(const std::string& command) override;
  void OverrideEntry(content::NavigationEntry* entry) override;
  void OnProceed() override;
  void OnDontProceed() override;

 private:
  void NotifyDenyCertificate();
  void NotifyAllowCertificate();

  base::Callback<void(content::CertificateRequestResultType)> callback_;

  content::WebContents* web_contents_;
  int cert_error_;
  const net::SSLInfo ssl_info_;
  GURL request_url_;
  std::string app_locale_;
  // Could the user successfully override the error?
  bool overridable_;
  // Has the site requested strict enforcement of certificate errors?
  bool strict_enforcement_;
  content::InterstitialPage* interstitial_page_;  // Owns us.
  // Is the hostname for an internal network?
  bool internal_;
  // Did the user previously allow a bad certificate but the decision has now
  // expired?
  const bool expired_but_previously_allowed_;

  DISALLOW_COPY_AND_ASSIGN(SSLBlockingPage);
};

#endif  // COMMON_ERROR_PAGES_SSL_SSL_BLOCKING_PAGE_H_
