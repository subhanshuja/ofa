// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_OPERA_FRAUD_PROTECTION_SERVICE_H_
#define CHILL_BROWSER_OPERA_FRAUD_PROTECTION_SERVICE_H_

#include "common/fraud_protection/fraud_protection_service.h"

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace opera {

class OperaFraudProtectionService : public FraudProtectionService {
 public:
  OperaFraudProtectionService(
      net::URLRequestContextGetter* request_context_getter);
  virtual ~OperaFraudProtectionService();

  bool IsEnabled() const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(OperaFraudProtectionService);
};

}  // namespace opera

#endif  // CHILL_BROWSER_OPERA_FRAUD_PROTECTION_SERVICE_H_
