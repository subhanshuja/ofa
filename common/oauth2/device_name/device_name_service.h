// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_DEVICE_NAME_DEVICE_NAME_SERVICE_H_
#define COMMON_OAUTH2_DEVICE_NAME_DEVICE_NAME_SERVICE_H_

#include <string>

#include "components/keyed_service/core/keyed_service.h"

namespace opera {
namespace oauth2 {

class DeviceNameService : public KeyedService {
 public:
  ~DeviceNameService() override {}

  virtual std::string GetCurrentDeviceName() const = 0;
  virtual std::string GetLastDeviceName() const = 0;
  virtual bool HasDeviceNameChanged() const = 0;
  virtual void StoreDeviceName(const std::string& device_name) = 0;
  virtual void ClearLastDeviceName() = 0;
};

}  // namespace oauth2
}  // namespace opera

#endif  // COMMON_OAUTH2_DEVICE_NAME_DEVICE_NAME_SERVICE_H_
