// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/device_name/device_name_service_impl.h"

#include "base/strings/string_util.h"
#include "base/sys_info.h"
#include "components/prefs/pref_service.h"

#include "common/oauth2/pref_names.h"
#include "common/oauth2/util/util.h"

namespace opera {
namespace oauth2 {

DeviceNameServiceImpl::DeviceNameServiceImpl(
    PrefService* pref_service,
    GetDeviceNameCallback get_device_name_callback)
    : pref_service_(pref_service),
      get_device_name_callback_(get_device_name_callback) {
  DCHECK(pref_service_);
  DCHECK(!get_device_name_callback_.is_null());
}

DeviceNameServiceImpl::~DeviceNameServiceImpl() {}

// static
void DeviceNameServiceImpl::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  DCHECK(registry);
  registry->RegisterStringPref(kOperaOAuth2LastDeviceNamePref, std::string());
}

std::string DeviceNameServiceImpl::GetCurrentDeviceName() const {
  DCHECK(!get_device_name_callback_.is_null());
  const auto device_name = get_device_name_callback_.Run();
  DCHECK(base::IsStringUTF8(device_name));
  return device_name;
}

std::string DeviceNameServiceImpl::GetLastDeviceName() const {
  return crypt::OSDecrypt(
      pref_service_->GetString(kOperaOAuth2LastDeviceNamePref));
}

bool DeviceNameServiceImpl::HasDeviceNameChanged() const {
  return (GetCurrentDeviceName() != GetLastDeviceName());
}

void DeviceNameServiceImpl::StoreDeviceName(const std::string& device_name) {
  pref_service_->SetString(kOperaOAuth2LastDeviceNamePref,
                           crypt::OSEncrypt(device_name));
}

void DeviceNameServiceImpl::ClearLastDeviceName() {
  pref_service_->ClearPref(kOperaOAuth2LastDeviceNamePref);
}

}  // namespace oauth2
}  // namespace opera
