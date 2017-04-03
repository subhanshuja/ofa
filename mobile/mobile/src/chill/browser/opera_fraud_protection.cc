// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/fraud_protection_ui/fraud_protection.h"

#include "chill/browser/opera_fraud_protection_delegate.h"

namespace opera {

/* static */
FraudProtectionDelegate* FraudProtection::CreateDelegate(
    FraudProtection* owner) {
  return new OperaFraudProtectionDelegate(owner);
}

}  // namespace opera
