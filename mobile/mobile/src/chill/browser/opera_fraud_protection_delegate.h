// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_OPERA_FRAUD_PROTECTION_DELEGATE_H_
#define CHILL_BROWSER_OPERA_FRAUD_PROTECTION_DELEGATE_H_

#include "base/compiler_specific.h"

#include "common/fraud_protection_ui/fraud_protection_delegate.h"

namespace opera {

class OperaFraudProtectionDelegate : public FraudProtectionDelegate {
 public:
  explicit OperaFraudProtectionDelegate(FraudProtection* owner);

  FraudProtectionService* GetFraudProtectionService(
      content::WebContents* web_contents) override;

  void OnTakeMeBackToStart(
      content::WebContents* web_contents) override;

 protected:
  virtual ~OperaFraudProtectionDelegate() {}
};

}  // namespace opera

#endif  // CHILL_BROWSER_OPERA_FRAUD_PROTECTION_DELEGATE_H_
