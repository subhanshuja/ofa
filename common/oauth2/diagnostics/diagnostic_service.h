// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_DIAGNOSTICS_DIAGNOSTIC_SERVICE_H_
#define COMMON_OAUTH2_DIAGNOSTICS_DIAGNOSTIC_SERVICE_H_

#include <deque>
#include <memory>
#include <vector>

#include "base/observer_list.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/keyed_service/core/keyed_service.h"

#include "common/oauth2/diagnostics/diagnostic_supplier.h"

namespace opera {
namespace oauth2 {
class DiagnosticService : public KeyedService {
 public:
  class Observer {
   public:
    virtual ~Observer();

    virtual void OnStateUpdate() = 0;
  };

  DiagnosticService(unsigned max_items);
  ~DiagnosticService() override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void AddSupplier(DiagnosticSupplier* supplier);
  void RemoveSupplier(DiagnosticSupplier* supplier);

  /**
   * To be called whenever the diagnostic state might have changed. The service
   * will query all suppliers for current diagnostic state and store the
   * combined response in case it differs from the last stored one.
   * In case the total stored state count exceeds |max_items_| the oldest
   * state will be removed.
   */
  void TakeSnapshot();

  /**
   * Returns a vector of gathered snapshots.
   */
  std::vector<std::unique_ptr<DiagnosticSupplier::Snapshot>> GetAllSnapshots()
      const;
  std::vector<std::unique_ptr<DiagnosticSupplier::Snapshot>>
  GetAllSnapshotsWithFormattedTimes() const;

  unsigned max_items() const;

 private:
  void FormatTimes(base::DictionaryValue* dict) const;

  unsigned max_items_;
  std::deque<std::unique_ptr<DiagnosticSupplier::Snapshot>> snapshots_;
  base::ObserverList<Observer> observers_;
  std::vector<DiagnosticSupplier*> suppliers_;
};
}  // namespace oauth2
}  // namespace opera

#endif  // COMMON_OAUTH2_DIAGNOSTICS_DIAGNOSTIC_SERVICE_H_
