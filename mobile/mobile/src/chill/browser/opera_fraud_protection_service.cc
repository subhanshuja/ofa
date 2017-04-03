// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/opera_fraud_protection_service.h"

#include "common/settings/settings_manager.h"

namespace opera {

OperaFraudProtectionService::OperaFraudProtectionService(
    net::URLRequestContextGetter* request_context_getter)
    : FraudProtectionService(request_context_getter) {
}

OperaFraudProtectionService::~OperaFraudProtectionService() {
}

bool OperaFraudProtectionService::IsEnabled() const {
  return SettingsManager::GetFraudProtection();
}

}  // namespace opera
