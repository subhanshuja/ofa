// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/about_sync_util.h"

#include "base/strings/utf_string_conversions.h"
#include "components/sync/driver/fake_sync_service.h"
#include "components/sync/engine/sync_status.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OPERA_SYNC)
#include <string>
#include "common/sync/sync_account_mock.h"
#endif  // OPERA_SYNC

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::_;

namespace syncer {
namespace sync_ui_util {
namespace {

class SyncServiceMock : public FakeSyncService {
 public:
  bool IsFirstSetupComplete() const override { return true; }

  bool HasUnrecoverableError() const override { return true; }

  bool QueryDetailedSyncStatus(SyncStatus* result) override { return false; }

  base::string16 GetLastSyncedTimeString() const override {
    return base::string16(base::ASCIIToUTF16("none"));
  }

  SyncCycleSnapshot GetLastCycleSnapshot() const override {
    return SyncCycleSnapshot();
  }
#if defined(OPERA_SYNC)
  std::string GetBackendInitializationStateString() const override {
    return "Not started";
  }

  opera::SyncAccount* account() const override { return sync_account_; }

  void SetAccount(opera::SyncAccount* account) { sync_account_ = account; }

  opera::SyncAccount* sync_account_ = nullptr;

#if defined(OPERA_DESKTOP)
  void OnSyncAuthKeeperInitialized() override {}
#endif  // OPERA_DESKTOP
#endif  // OPERA_SYNC
};

TEST(SyncUIUtilTestAbout, ConstructAboutInformationWithUnrecoverableErrorTest) {
  SyncServiceMock service;

#if defined(OPERA_SYNC)
  // TODO(mpawlowski) provide non-null DuplicateTracker?
  std::unique_ptr<base::DictionaryValue> strings(ConstructAboutInformation(
      &service, nullptr, version_info::Channel::UNKNOWN));
#else
  std::unique_ptr<base::DictionaryValue> strings(ConstructAboutInformation(
      &service, NULL, version_info::Channel::UNKNOWN));
#endif  // OPERA_SYNC

  EXPECT_TRUE(strings->HasKey("unrecoverable_error_detected"));
}
}  // namespace
}  // namespace sync_ui_util
}  // namespace syncer
