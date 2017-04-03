// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/opera_fraud_protection_delegate.h"

#include <string>

#include "content/public/browser/web_contents.h"

#include "chill/browser/chromium_tab.h"

namespace opera {

OperaFraudProtectionDelegate::OperaFraudProtectionDelegate(
    FraudProtection* owner)
    : FraudProtectionDelegate(owner) {}

FraudProtectionService* OperaFraudProtectionDelegate::GetFraudProtectionService(
    content::WebContents* web_contents) {
  ChromiumTab* tab = ChromiumTab::FromWebContents(web_contents);
  return tab->GetFraudProtectionService();
}

void OperaFraudProtectionDelegate::OnTakeMeBackToStart(
    content::WebContents* web_contents) {
  // This method is called when there's no history to go back to. For
  // this case, close the tab since we can't navigate to dashboard
  // page within it.
  web_contents->ClosePage();
}

}  // namespace opera
