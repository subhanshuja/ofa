// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/fraud_protection_ui/fraud_protection_delegate.h"
#include "common/fraud_protection_ui/fraud_warning_page.h"

namespace opera {

FraudProtectionDelegate::FraudProtectionDelegate(FraudProtection* owner)
    : fraud_protection_(owner) {}

FraudWarningPage* FraudProtectionDelegate::CreateFraudWarningPage(
    content::WebContents* web_contents, const FraudUrlRating& rating) {
  return new FraudWarningPage(web_contents, rating, this);
}

}  // namespace opera
