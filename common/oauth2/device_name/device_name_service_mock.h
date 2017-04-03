// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_DEVICE_NAME_DEVICE_NAME_SERVICE_MOCK_H_
#define COMMON_OAUTH2_DEVICE_NAME_DEVICE_NAME_SERVICE_MOCK_H_

#include <string>

#include "testing/gmock/include/gmock/gmock.h"

#include "common/oauth2/device_name/device_name_service.h"

namespace opera {
namespace oauth2 {

class DeviceNameServiceMock : public DeviceNameService {
 public:
  DeviceNameServiceMock();
  ~DeviceNameServiceMock() override;

  MOCK_CONST_METHOD0(GetCurrentDeviceName, std::string());
  MOCK_CONST_METHOD0(GetLastDeviceName, std::string());
  MOCK_CONST_METHOD0(HasDeviceNameChanged, bool());
  MOCK_METHOD1(StoreDeviceName, void(const std::string&));
  MOCK_METHOD0(ClearLastDeviceName, void());
};
}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_DEVICE_NAME_DEVICE_NAME_SERVICE_MOCK_H_
