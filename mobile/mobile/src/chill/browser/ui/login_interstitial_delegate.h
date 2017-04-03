// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// NOLINTNEXTLINE(whitespace/line_length)
// @copied-from chromium/src/chrome/browser/ui/login/login_interstitial_delegate.h
// @last-synchronized e4e0b36f3e5b582e3a09de5d1a05064f22fff91e

#ifndef CHILL_BROWSER_UI_LOGIN_INTERSTITIAL_DELEGATE_H_
#define CHILL_BROWSER_UI_LOGIN_INTERSTITIAL_DELEGATE_H_

#include <string>

#include "base/callback.h"
#include "content/public/browser/interstitial_page.h"
#include "content/public/browser/interstitial_page_delegate.h"
#include "url/gurl.h"

namespace content {
class WebContents;
}

namespace opera {


// Placeholder interstitial for HTTP login prompts. This interstitial makes the
// omnibox show the correct url when the login prompt is visible.
class LoginInterstitialDelegate : public content::InterstitialPageDelegate {
 public:
  LoginInterstitialDelegate(content::WebContents* web_contents,
                            const GURL& request_url,
                            const base::Closure& callback);

  virtual ~LoginInterstitialDelegate();

  void CommandReceived(const std::string& command) override;

  void OnProceed() override;

  void OnDontProceed() override;

 protected:
  std::string GetHTMLContents() override;

 private:
  base::Closure callback_;
  DISALLOW_COPY_AND_ASSIGN(LoginInterstitialDelegate);
};

}  // namespace opera

#endif  // CHILL_BROWSER_UI_LOGIN_INTERSTITIAL_DELEGATE_H_
