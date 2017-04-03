// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_SYNC_INVALIDATION_DATA_H_
#define MOBILE_COMMON_SYNC_INVALIDATION_DATA_H_

#include <string>

#include "base/observer_list.h"
#include "mobile/common/sync/sync_manager.h"

namespace mobile {

class InvalidationData {
 public:
  class Observer {
   public:
    virtual void InvalidationDataLoaded(InvalidationData* data) = 0;
    virtual void InvalidationDataChanged(InvalidationData* data) = 0;
   protected:
    virtual ~Observer() {};
  };

  bool IsLoaded() const {
    return loaded_;
  }

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  SyncManager::ServiceType GetServiceType() const {
    return service_type_;
  }

  SyncManager::Build GetBuild() const {
    return build_;
  }

  std::string GetDeviceId() const {
    return device_id_;
  }

 protected:
  InvalidationData(SyncManager::ServiceType service_type,
                   SyncManager::Build build);
  virtual ~InvalidationData();

  void Loaded(const std::string& new_device_id);
  void Changed(const std::string& new_device_id);

 private:
  const SyncManager::ServiceType service_type_;
  const SyncManager::Build build_;
  std::string device_id_;
  bool loaded_;
  base::ObserverList<Observer> observers_;
};

}  // namespace mobile

#endif  // MOBILE_COMMON_SYNC_INVALIDATION_DATA_H_
