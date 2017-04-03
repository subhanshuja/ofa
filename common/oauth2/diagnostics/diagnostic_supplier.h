// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_DIAGNOSTICS_DIAGNOSTIC_SUPPLIER_H_
#define COMMON_OAUTH2_DIAGNOSTICS_DIAGNOSTIC_SUPPLIER_H_

#include <memory>

#include "base/time/time.h"
#include "base/values.h"

namespace opera {
namespace oauth2 {

class DiagnosticSupplier {
 public:
  struct Snapshot {
    Snapshot();
    ~Snapshot();

    base::Time timestamp;
    std::unique_ptr<base::DictionaryValue> state;
  };

  virtual ~DiagnosticSupplier() {}

  virtual std::string GetDiagnosticName() const = 0;
  virtual std::unique_ptr<base::DictionaryValue> GetDiagnosticSnapshot() = 0;
};
}  // namespace oauth2
}  // namespace opera

#endif  // COMMON_OAUTH2_DIAGNOSTICS_DIAGNOSTIC_SUPPLIER_H_
