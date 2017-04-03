
// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/device_name/device_name_service_impl.h"

#include "base/memory/ptr_util.h"
#include "components/os_crypt/os_crypt.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "common/oauth2/network/access_token_request.h"
#include "common/oauth2/util/util.h"
#include "common/oauth2/pref_names.h"
#include "common/oauth2/test/testing_constants.h"

namespace opera {
namespace oauth2 {
namespace {

const std::string kOldDeviceName = "Albratross";
const std::string kCurrentDeviceName = "Equador";

using ::testing::_;

class TestingGetSessionNameImplSupplier {
 public:
  void SetSuppliedDeviceName(const std::string& device_name) {
    supplied_device_name_ = device_name;
  }

  std::string ReturnSuppliedDeviceName() const { return supplied_device_name_; }

 protected:
  std::string supplied_device_name_;
};

class DeviceNameServiceImplTest : public ::testing::Test {
 public:
  DeviceNameServiceImplTest() {
#if defined(OS_MACOSX)
    OSCrypt::UseMockKeychain(true);
#endif  // defined(OS_MACOSX)
  }
  ~DeviceNameServiceImplTest() override {}

  void SetUp() override {
    testing_pref_service_.registry()->RegisterStringPref(
        kOperaOAuth2LastDeviceNamePref, std::string());

    device_name_service_.reset(new DeviceNameServiceImpl(
        &testing_pref_service_,
        base::Bind(&TestingGetSessionNameImplSupplier::ReturnSuppliedDeviceName,
                   base::Unretained(&name_supplier_))));
  }

  std::string GetDeviceNameFromPrefsAndDecrypt() const {
    return crypt::OSDecrypt(
        testing_pref_service_.GetString(kOperaOAuth2LastDeviceNamePref));
  }

 protected:
  std::unique_ptr<DeviceNameService> device_name_service_;
  TestingGetSessionNameImplSupplier name_supplier_;
  TestingPrefServiceSimple testing_pref_service_;
};
}  // namespace

TEST_F(DeviceNameServiceImplTest, ReturnsCurrent) {
  name_supplier_.SetSuppliedDeviceName(kCurrentDeviceName);
  ASSERT_EQ(kCurrentDeviceName, device_name_service_->GetCurrentDeviceName());
}

TEST_F(DeviceNameServiceImplTest, HasChangedWorks) {
  name_supplier_.SetSuppliedDeviceName(kCurrentDeviceName);
  ASSERT_EQ(kCurrentDeviceName, device_name_service_->GetCurrentDeviceName());
  ASSERT_EQ(std::string(), device_name_service_->GetLastDeviceName());
  ASSERT_EQ(true, device_name_service_->HasDeviceNameChanged());
  device_name_service_->StoreDeviceName(kCurrentDeviceName);
  ASSERT_EQ(kCurrentDeviceName, GetDeviceNameFromPrefsAndDecrypt());
  ASSERT_EQ(kCurrentDeviceName, device_name_service_->GetLastDeviceName());
  EXPECT_FALSE(device_name_service_->HasDeviceNameChanged());
}

TEST_F(DeviceNameServiceImplTest, ClearWorks) {
  name_supplier_.SetSuppliedDeviceName(kCurrentDeviceName);
  ASSERT_EQ(kCurrentDeviceName, device_name_service_->GetCurrentDeviceName());
  device_name_service_->StoreDeviceName(kCurrentDeviceName);
  ASSERT_EQ(kCurrentDeviceName, GetDeviceNameFromPrefsAndDecrypt());
  ASSERT_EQ(kCurrentDeviceName, device_name_service_->GetLastDeviceName());
  device_name_service_->ClearLastDeviceName();
  ASSERT_EQ(std::string(), device_name_service_->GetLastDeviceName());
}

TEST_F(DeviceNameServiceImplTest, RequestRace) {
  ASSERT_EQ(std::string(), GetDeviceNameFromPrefsAndDecrypt());
  // Simulate two network requests that update the device name in prefs.
  // Scenario:
  // 1. Empty prefs, current device name is "Albratross";
  name_supplier_.SetSuppliedDeviceName(kOldDeviceName);
  // 2. Request A starts with "Albratross" and hangs due to network conditions;
  ASSERT_EQ(kOldDeviceName, device_name_service_->GetCurrentDeviceName());
  // 3. Device name changes to "Equador";
  name_supplier_.SetSuppliedDeviceName(kCurrentDeviceName);
  // 4. Request B starts with "Equador";
  ASSERT_EQ(kCurrentDeviceName, device_name_service_->GetCurrentDeviceName());
  // 5. Request B finishes and updates prefs with "Equador";
  device_name_service_->StoreDeviceName(kCurrentDeviceName);
  ASSERT_EQ(kCurrentDeviceName, GetDeviceNameFromPrefsAndDecrypt());
  // 6. Request A finishes and restores "Albatross";
  device_name_service_->StoreDeviceName(kOldDeviceName);
  // 7. Everything ready for the next request to include "Equador"
  EXPECT_TRUE(device_name_service_->HasDeviceNameChanged());
  EXPECT_EQ(kCurrentDeviceName, device_name_service_->GetCurrentDeviceName());
  EXPECT_EQ(kOldDeviceName, device_name_service_->GetLastDeviceName());
}

}  // namespace oauth2
}  // namespace opera
