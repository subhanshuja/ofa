// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/sync/invalidation_data.h"

namespace mobile {

InvalidationData::InvalidationData(SyncManager::ServiceType service_type,
                                   SyncManager::Build build)
    : service_type_(service_type), build_(build), loaded_(false) {
}

InvalidationData::~InvalidationData() {
}

void InvalidationData::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void InvalidationData::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void InvalidationData::Loaded(const std::string& new_device_id) {
  DCHECK(!loaded_);
  loaded_ = true;
  device_id_ = new_device_id;
  FOR_EACH_OBSERVER(Observer, observers_, InvalidationDataLoaded(this));
}

void InvalidationData::Changed(const std::string& new_device_id) {
  DCHECK(loaded_);
  if (device_id_ != new_device_id) {
    device_id_ = new_device_id;
    FOR_EACH_OBSERVER(Observer, observers_, InvalidationDataChanged(this));
  }
}

}  // namespace mobile
