// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_DEVICE_NAME_DEVICE_NAME_SERVICE_IMPL_H_
#define COMMON_OAUTH2_DEVICE_NAME_DEVICE_NAME_SERVICE_IMPL_H_

#include "common/oauth2/device_name/device_name_service.h"

#include <string>

#include "base/callback.h"
#include "components/pref_registry/pref_registry_syncable.h"

class PrefService;

namespace opera {
namespace oauth2 {

class DeviceNameServiceImpl : public DeviceNameService {
 public:
  typedef base::Callback<std::string()> GetDeviceNameCallback;

  DeviceNameServiceImpl(PrefService* pref_service,
                        GetDeviceNameCallback get_device_name_callback);
  ~DeviceNameServiceImpl() override;

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  std::string GetCurrentDeviceName() const override;
  std::string GetLastDeviceName() const override;
  bool HasDeviceNameChanged() const override;
  void StoreDeviceName(const std::string& device_name) override;
  void ClearLastDeviceName() override;

 private:
  PrefService* pref_service_;
  GetDeviceNameCallback get_device_name_callback_;
};

}  // namespace oauth2
}  // namespace opera

#endif  // COMMON_OAUTH2_DEVICE_NAME_DEVICE_NAME_SERVICE_IMPL_H_
