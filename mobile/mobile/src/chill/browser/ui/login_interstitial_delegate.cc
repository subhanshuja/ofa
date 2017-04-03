// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// NOLINTNEXTLINE(whitespace/line_length)
// @copied-from chromium/src/chrome/browser/ui/login/login_interstitial_delegate.cc
// @last-synchronized e4e0b36f3e5b582e3a09de5d1a05064f22fff91e

#include "chill/browser/ui/login_interstitial_delegate.h"

#include "content/public/browser/interstitial_page.h"

namespace opera {

LoginInterstitialDelegate::LoginInterstitialDelegate(
    content::WebContents* web_contents,
    const GURL& request_url,
    const base::Closure& callback)
    : callback_(callback) {
  // The interstitial page owns us.
  content::InterstitialPage* interstitial_page =
      content::InterstitialPage::Create(web_contents,
                                        true,
                                        request_url,
                                        this);
  interstitial_page->Show();
}

LoginInterstitialDelegate::~LoginInterstitialDelegate() {
}

void LoginInterstitialDelegate::CommandReceived(const std::string& command) {
  callback_.Run();
}

void LoginInterstitialDelegate::OnProceed() {
  InterstitialPageDelegate::OnProceed();
}

void LoginInterstitialDelegate::OnDontProceed() {
  InterstitialPageDelegate::OnDontProceed();
}

std::string LoginInterstitialDelegate::GetHTMLContents() {
  // Showing an interstitial results in a new navigation, and a new navigation
  // closes all modal dialogs on the page. Therefore the login prompt must be
  // shown after the interstitial is displayed. This is done by sending a
  // command from the interstitial page as soon as it is loaded.
  return std::string(
      "<!DOCTYPE html>"
      "<html><body><script>"
      "window.domAutomationController.setAutomationId(1);"
      "window.domAutomationController.send('1');"
      "</script></body></html>");
}

}  // namespace opera
