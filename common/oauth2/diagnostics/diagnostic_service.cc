// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/diagnostics/diagnostic_service.h"

#include "base/memory/ptr_util.h"
#include "base/strings/string_util.h"
#include "components/sync/api/time.h"

namespace opera {
namespace oauth2 {
DiagnosticService::Observer::~Observer() {}

DiagnosticService::DiagnosticService(unsigned max_items)
    : max_items_(max_items) {
  DCHECK_GT(max_items, 0u);
}

DiagnosticService::~DiagnosticService() {}

void DiagnosticService::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void DiagnosticService::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void DiagnosticService::AddSupplier(DiagnosticSupplier* supplier) {
  DCHECK(supplier);
  DCHECK(std::find(suppliers_.begin(), suppliers_.end(), supplier) ==
         suppliers_.end());
  DCHECK(!supplier->GetDiagnosticName().empty());
  suppliers_.push_back(supplier);
}

void DiagnosticService::RemoveSupplier(DiagnosticSupplier* supplier) {
  DCHECK(supplier);
  DCHECK(std::find(suppliers_.begin(), suppliers_.end(), supplier) !=
         suppliers_.end());
  suppliers_.erase(std::find(suppliers_.begin(), suppliers_.end(), supplier));
}

void DiagnosticService::TakeSnapshot() {
  DCHECK_LE(snapshots_.size(), max_items_);
  std::unique_ptr<base::DictionaryValue> snapshot(new base::DictionaryValue);
  for (const auto& s : suppliers_) {
    auto subsnapshot = s->GetDiagnosticSnapshot();
    if (subsnapshot->empty()) {
      continue;
    }
    snapshot->Set(s->GetDiagnosticName(), std::move(subsnapshot));
  }

  if (snapshot->empty()) {
    return;
  }

  if (snapshots_.size() > 0) {
    const auto& first = snapshots_.front();
    if (snapshot->Equals(first->state.get())) {
      return;
    }
  }

  std::unique_ptr<DiagnosticSupplier::Snapshot> new_snapshot(
      new DiagnosticSupplier::Snapshot);
  new_snapshot->timestamp = base::Time::Now();
  new_snapshot->state = std::move(snapshot);
  snapshots_.push_front(std::move(new_snapshot));
  if (snapshots_.size() > max_items_) {
    snapshots_.pop_back();
  }

  FOR_EACH_OBSERVER(Observer, observers_, OnStateUpdate());
}

unsigned DiagnosticService::max_items() const {
  return max_items_;
}

std::vector<std::unique_ptr<DiagnosticSupplier::Snapshot>>
DiagnosticService::GetAllSnapshots() const {
  std::vector<std::unique_ptr<DiagnosticSupplier::Snapshot>> list;
  for (const auto& snapshot : snapshots_) {
    std::unique_ptr<DiagnosticSupplier::Snapshot> s(
        new DiagnosticSupplier::Snapshot);
    s->timestamp = snapshot->timestamp;
    s->state = snapshot->state->CreateDeepCopy();
    list.push_back(std::move(s));
  }
  return list;
}

std::vector<std::unique_ptr<DiagnosticSupplier::Snapshot>>
DiagnosticService::GetAllSnapshotsWithFormattedTimes() const {
  auto snapshots = GetAllSnapshots();
  for (auto it = snapshots.begin(); it != snapshots.end(); it++) {
    DiagnosticSupplier::Snapshot* snapshot = it->get();
    DCHECK(snapshot);
    base::DictionaryValue* dict = snapshot->state.get();
    DCHECK(dict);
    base::DictionaryValue::Iterator dict_it(*dict);
    while (!dict_it.IsAtEnd()) {
      DCHECK(dict_it.value().IsType(base::Value::TYPE_DICTIONARY));
      base::DictionaryValue* dict_value;
      bool ok = dict->GetDictionary(dict_it.key(), &dict_value);
      DCHECK(ok);
      FormatTimes(dict_value);
      dict_it.Advance();
    }
  }
  return snapshots;
}

void DiagnosticService::FormatTimes(base::DictionaryValue* dict) const {
  base::DictionaryValue::Iterator it(*dict);
  while (!it.IsAtEnd()) {
    if (base::EndsWith(it.key(), "_time",
                       base::CompareCase::INSENSITIVE_ASCII)) {
      double js_time = 0;
      bool ok = it.value().GetAsDouble(&js_time);
      DCHECK(ok);
      dict->SetString(it.key() + "_str", syncer::GetTimeDebugString(
                                             base::Time::FromJsTime(js_time)));
    } else if (it.value().IsType(base::Value::TYPE_DICTIONARY)) {
      base::DictionaryValue* subdict = nullptr;
      bool ok = dict->GetDictionary(it.key(), &subdict);
      DCHECK(ok);
      DCHECK(subdict);
      FormatTimes(subdict);
    } else if (it.value().IsType(base::Value::TYPE_LIST)) {
      base::ListValue* sublist = nullptr;
      bool ok = dict->GetList(it.key(), &sublist);
      DCHECK(ok);
      DCHECK(sublist);
      for (const auto& list_element : *sublist) {
        if (list_element->IsType(base::Value::TYPE_DICTIONARY)) {
          base::DictionaryValue* subdict = nullptr;
          bool ok = list_element->GetAsDictionary(&subdict);
          DCHECK(ok);
          DCHECK(subdict);
          FormatTimes(subdict);
        }
      }
    }
    it.Advance();
  }
}

}  // namespace oauth2
}  // namespace opera
