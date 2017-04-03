// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/device_info/local_device_info_provider_impl.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "components/sync/base/get_session_name.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OPERA_DESKTOP)
#include "third_party/re2/src/re2/re2.h"
#include "desktop/common/opera_version_info.h"
#endif  // OPERA_DESKTOP

namespace syncer {

const char kLocalDeviceGuid[] = "foo";
const char kSigninScopedDeviceId[] = "device_id";

class LocalDeviceInfoProviderImplTest : public testing::Test {
 public:
  LocalDeviceInfoProviderImplTest() : called_back_(false) {}
  ~LocalDeviceInfoProviderImplTest() override {}

  void SetUp() override {
    provider_.reset(new LocalDeviceInfoProviderImpl(
        version_info::Channel::UNKNOWN,
        version_info::GetVersionStringWithModifier("UNKNOWN"), false));
  }

  void TearDown() override {
    provider_.reset();
    called_back_ = false;
  }

 protected:
  void StartInitializeProvider() { StartInitializeProvider(kLocalDeviceGuid); }

  void StartInitializeProvider(const std::string& guid) {
    provider_->Initialize(guid, kSigninScopedDeviceId,
                          message_loop_.task_runner());
  }

  void FinishInitializeProvider() {
    // Subscribe to the notification and wait until the callback
    // is called. The callback will quit the loop.
    base::RunLoop run_loop;
    std::unique_ptr<LocalDeviceInfoProvider::Subscription> subscription =
        provider_->RegisterOnInitializedCallback(
            base::Bind(&LocalDeviceInfoProviderImplTest::QuitLoopOnInitialized,
                       base::Unretained(this), &run_loop));
    run_loop.Run();
  }

  void InitializeProvider() {
    StartInitializeProvider();
    FinishInitializeProvider();
  }

  void QuitLoopOnInitialized(base::RunLoop* loop) {
    called_back_ = true;
    loop->Quit();
  }

  std::unique_ptr<LocalDeviceInfoProviderImpl> provider_;

  bool called_back_;

 private:
  base::MessageLoop message_loop_;
};

TEST_F(LocalDeviceInfoProviderImplTest, OnInitializedCallback) {
  ASSERT_FALSE(called_back_);
  StartInitializeProvider();
  ASSERT_FALSE(called_back_);
  FinishInitializeProvider();
  EXPECT_TRUE(called_back_);
}

TEST_F(LocalDeviceInfoProviderImplTest, GetLocalDeviceInfo) {
  ASSERT_EQ(nullptr, provider_->GetLocalDeviceInfo());
  StartInitializeProvider();
  ASSERT_EQ(nullptr, provider_->GetLocalDeviceInfo());
  FinishInitializeProvider();

  const DeviceInfo* local_device_info = provider_->GetLocalDeviceInfo();
  ASSERT_NE(nullptr, local_device_info);
  EXPECT_EQ(std::string(kLocalDeviceGuid), local_device_info->guid());
  EXPECT_EQ(std::string(kSigninScopedDeviceId),
            local_device_info->signin_scoped_device_id());
  EXPECT_EQ(GetSessionNameSynchronouslyForTesting(),
            local_device_info->client_name());

  EXPECT_EQ(provider_->GetSyncUserAgent(),
            local_device_info->sync_user_agent());

  provider_->Clear();
  ASSERT_EQ(nullptr, provider_->GetLocalDeviceInfo());
}

TEST_F(LocalDeviceInfoProviderImplTest, GetLocalSyncCacheGUID) {
  EXPECT_TRUE(provider_->GetLocalSyncCacheGUID().empty());

  StartInitializeProvider();
  EXPECT_EQ(std::string(kLocalDeviceGuid), provider_->GetLocalSyncCacheGUID());

  FinishInitializeProvider();
  EXPECT_EQ(std::string(kLocalDeviceGuid), provider_->GetLocalSyncCacheGUID());

  provider_->Clear();
  EXPECT_TRUE(provider_->GetLocalSyncCacheGUID().empty());
}

TEST_F(LocalDeviceInfoProviderImplTest, InitClearRace) {
  EXPECT_TRUE(provider_->GetLocalSyncCacheGUID().empty());
  StartInitializeProvider();

  provider_->Clear();
  ASSERT_EQ(nullptr, provider_->GetLocalDeviceInfo());
  EXPECT_TRUE(provider_->GetLocalSyncCacheGUID().empty());

  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(nullptr, provider_->GetLocalDeviceInfo());
  EXPECT_TRUE(provider_->GetLocalSyncCacheGUID().empty());
}

TEST_F(LocalDeviceInfoProviderImplTest, InitClearInitRace) {
  EXPECT_TRUE(provider_->GetLocalSyncCacheGUID().empty());
  StartInitializeProvider();
  provider_->Clear();

  const std::string guid2 = "guid2";
  StartInitializeProvider(guid2);
  ASSERT_EQ(nullptr, provider_->GetLocalDeviceInfo());
  EXPECT_EQ(guid2, provider_->GetLocalSyncCacheGUID());

  FinishInitializeProvider();
  const DeviceInfo* local_device_info = provider_->GetLocalDeviceInfo();
  ASSERT_NE(nullptr, local_device_info);
  EXPECT_EQ(guid2, local_device_info->guid());
  EXPECT_EQ(guid2, provider_->GetLocalSyncCacheGUID());
}

TEST_F(LocalDeviceInfoProviderTest, SyncUserAgentMatchesWhatWeNeed) {
  // This test checks that the "sync UA string" matches what we expect.
  // Note that the string value depends directly on the browser version
  // configured via the VERSION file.
  // Note that the version string may actually take two forms, the short
  // and long, i.e. a canonical version string format is:
  // MAJOR.MINOR.NIGHTLY.PATCH.BUILD
  // The above is what we consider a long version string here.
  // The BUILD however may get ommitted completely along with its preceeding
  // dot and is assumed to be 0 then. What we get then is the short version:
  // MAJOR.MINOR.NIGHTLY.PATCH
  ASSERT_EQ(NULL, provider_->GetLocalDeviceInfo());
  InitializeProvider();

  const DeviceInfo* local_device_info = provider_->GetLocalDeviceInfo();
  EXPECT_TRUE(!!local_device_info);
  const std::string user_agent = local_device_info->sync_user_agent();

  std::string browser, platform, name, uaname, channel;
  int major_a = -1, minor_a = -1, nigtly_a = -1, patch_a = -1, bnum_a = -1;
  // The UAVersion does not contain the build number even if it is set up.
  int major_b = -1, minor_b = -1, nigtly_b = -1, patch_b = -1;
  bool result = false;
  if (opera_version_info::VersionBuild() == 0) {
    // Short version, build number defaults to 0.
    // Example:
    // Opera WIN 27.0.1695.0 (Opera OPR/27.0.1695.0) channel(developer)
    bnum_a = 0;
    result = RE2::FullMatch(
        user_agent,
        "(\\w+) ([A-Z]+) (\\d+)\\.(\\d+)\\.(\\d+)\\.(\\d+) \\((\\w+) ([A-Z]+)"
        "/(\\d+)\\.(\\d+)\\.(\\d+)\\.(\\d+)\\) channel\\((.*)\\)",
        &browser,
        &platform,
        &major_a,
        &minor_a,
        &nigtly_a,
        &patch_a,
        &name,
        &uaname,
        &major_b,
        &minor_b,
        &nigtly_b,
        &patch_b,
        &channel);
  } else {
    // Long version.
    // Example:
    // Opera WIN 27.0.1695.0.51 (Opera OPR/27.0.1695.0) channel(developer)
    result = RE2::FullMatch(
        user_agent,
        "(\\w+) ([A-Z]+) (\\d+)\\.(\\d+)\\.(\\d+)\\.(\\d+)\\.(\\d+) \\((\\w+)"
        " ([A-Z]+)/(\\d+)\\.(\\d+)\\.(\\d+)\\.(\\d+)\\) channel\\((.*)\\)",
        &browser,
        &platform,
        &major_a,
        &minor_a,
        &nigtly_a,
        &patch_a,
        &bnum_a,
        &name,
        &uaname,
        &major_b,
        &minor_b,
        &nigtly_b,
        &patch_b,
        &channel);
  }
  EXPECT_TRUE(result) << "'" << user_agent << "' vs '"
                      << opera_version_info::GetVersionNumber() << "'";

  EXPECT_EQ(browser, "Opera");

#if defined(OS_WIN)
  EXPECT_EQ(platform, "WIN");
#elif defined(OS_LINUX)
  EXPECT_EQ(platform, "LINUX");
#elif defined(OS_MACOSX)
  EXPECT_EQ(platform, "MAC");
#else
  NOTREACHED() << "Align with "
      "LocalDeviceInfoProviderImpl::MakeUserAgentForSyncApi() "
      "for new platforms.";
#endif

  EXPECT_EQ(major_a, opera_version_info::VersionMajor());
  EXPECT_EQ(minor_a, opera_version_info::VersionMinor());
  EXPECT_EQ(nigtly_a, opera_version_info::VersionNightly());
  EXPECT_EQ(patch_a, opera_version_info::VersionPatch());
  EXPECT_EQ(bnum_a, opera_version_info::VersionBuild());

  EXPECT_EQ(name, opera_version_info::GetProductName());
  EXPECT_EQ(uaname, opera_version_info::GetUserAgentName());

  EXPECT_EQ(major_b, opera_version_info::VersionMajor());
  EXPECT_EQ(minor_b, opera_version_info::VersionMinor());
  EXPECT_EQ(nigtly_b, opera_version_info::VersionNightly());
  EXPECT_EQ(patch_b, opera_version_info::VersionPatch());

  EXPECT_EQ(channel, opera_version_info::GetChannelString());
}

}  // namespace syncer
