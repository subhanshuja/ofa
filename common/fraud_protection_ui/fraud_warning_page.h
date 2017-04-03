// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_FRAUD_PROTECTION_UI_FRAUD_WARNING_PAGE_H_
#define COMMON_FRAUD_PROTECTION_UI_FRAUD_WARNING_PAGE_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/observer_list.h"
#include "content/public/browser/interstitial_page_delegate.h"
#include "url/gurl.h"

#include "common/fraud_protection/fraud_url_rating.h"

namespace content {
class InterstitialPage;
class WebContents;
}

namespace opera {

class FraudProtectionDelegate;

class FraudWarningPage : public content::InterstitialPageDelegate {
 public:
  class Observer {
   public:
    virtual void OnFraudWarningShown(content::WebContents*) = 0;
    virtual void OnFraudWarningDismissed(content::WebContents*) = 0;
  };

  explicit FraudWarningPage(
      content::WebContents* web_contents,
      const FraudUrlRating& rating,
      FraudProtectionDelegate* delegate);
  ~FraudWarningPage() override;

  std::string GetHTMLContents() override;
  void CommandReceived(const std::string& command) override;
  void SetNeedsToReloadOnProceed() { needs_to_reload_on_proceeding_ = true; }

  void Show();
  void AddObserver(Observer* observer) { observers_.AddObserver(observer); }
  void RemoveObserver(Observer* observer) {
    observers_.RemoveObserver(observer);
  }

 private:
  bool needs_to_reload_on_proceeding_;
  content::WebContents* web_contents_;
  content::InterstitialPage* page_;
  FraudUrlRating rating_;
  FraudProtectionDelegate* delegate_;
  std::string advisory_homepage_;

  // Observers
  base::ObserverList<FraudWarningPage::Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(FraudWarningPage);
};

}  // namespace opera

#endif  // COMMON_FRAUD_PROTECTION_UI_FRAUD_WARNING_PAGE_H_
