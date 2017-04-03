// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_FRAUD_PROTECTION_UI_FRAUD_PROTECTION_DELEGATE_H_
#define COMMON_FRAUD_PROTECTION_UI_FRAUD_PROTECTION_DELEGATE_H_

#include "base/callback_forward.h"

class GURL;

namespace content {
class WebContents;
}

namespace opera {

class FraudProtection;
class FraudProtectionService;
class FraudUrlRating;
class FraudWarningPage;

// This interface is used to integrate with the fraud protection system.
class FraudProtectionDelegate {
 public:
  explicit FraudProtectionDelegate(FraudProtection* owner);
  virtual ~FraudProtectionDelegate() {}

  virtual FraudWarningPage* CreateFraudWarningPage(
      content::WebContents* web_contents,
      const FraudUrlRating& rating);

  // Returns the fraud protection service to use for a specific web contents.
  virtual FraudProtectionService* GetFraudProtectionService(
      content::WebContents* web_contents) = 0;

  // Called when the user want's to be taken back to the start page.
  virtual void OnTakeMeBackToStart(content::WebContents* web_contents) = 0;

  // Asks the delegate for starting or stopping (depends on |notify|) to notify
  // |fraud_protection_| about the fact of blocked navigations.
  // A navigation could be blocked e.g. due to showing the http auth
  // or the invalid certificate notification.
  virtual void NotifyAboutNavigationBlocked(content::WebContents* web_contents,
                                            bool notify) {}

  // When called the navigation blocking notifiaction must be dismissed.
  virtual void ForceUnblockingNavigation(content::WebContents* web_contents) {}

 protected:
  FraudProtection* fraud_protection_;
};

}  // namespace opera

#endif  // COMMON_FRAUD_PROTECTION_UI_FRAUD_PROTECTION_DELEGATE_H_
