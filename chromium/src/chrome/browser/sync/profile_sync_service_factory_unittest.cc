// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/profile_sync_service_factory.h"

#include <stddef.h>

#include <memory>
#include <vector>

#include "base/command_line.h"
#include "build/build_config.h"
#include "chrome/test/base/testing_profile.h"
#include "components/browser_sync/browser_sync_switches.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/sync/base/model_type.h"
#include "components/sync/driver/data_type_controller.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/app_list/app_list_switches.h"

#if defined(OPERA_SYNC)
#include "common/oauth2/token_cache/webdata_client_factory.h"
#include "common/sync/profile_sync_service_params_provider_provider_mock.h"
#endif  // OPERA_SYNC

using browser_sync::ProfileSyncService;
using syncer::DataTypeController;

class ProfileSyncServiceFactoryTest : public testing::Test {
 protected:
  ProfileSyncServiceFactoryTest() : profile_(new TestingProfile()) {}

#if defined(OPERA_SYNC)
  void SetUp() override {
    opera::ProfileSyncServiceParamsProvider::GetInstance()->set_provider(
        &opera_pss_params_provider_mock_);
    opera::oauth2::WebdataClientFactory::GetInstance()->SetTestingFactory(
        profile(),
        &opera::oauth2::WebdataClientFactory::BuildTestingServiceInstanceFor);
  }
#endif  // OPERA_SYNC

  // Returns the collection of default datatypes.
  std::vector<syncer::ModelType> DefaultDatatypes() {
    std::vector<syncer::ModelType> datatypes;

    // Desktop types.
#if !defined(OS_ANDROID)
#if !defined(OPERA_SYNC)
    datatypes.push_back(syncer::APPS);
#endif  // !OPERA_SYNC
#if defined(ENABLE_APP_LIST)
#if !defined(OPERA_SYNC)
    if (app_list::switches::IsAppListSyncEnabled())
      datatypes.push_back(syncer::APP_LIST);
#endif  // !OPERA_SYNC
#endif
#if !defined(OPERA_SYNC)
    datatypes.push_back(syncer::APP_SETTINGS);
#if defined(OS_LINUX) || defined(OS_WIN) || defined(OS_CHROMEOS)
    datatypes.push_back(syncer::DICTIONARY);
#endif
#if defined(OS_CHROMEOS)
    datatypes.push_back(syncer::ARC_PACKAGE);
#endif
    datatypes.push_back(syncer::EXTENSIONS);
    datatypes.push_back(syncer::EXTENSION_SETTINGS);
    datatypes.push_back(syncer::SEARCH_ENGINES);
    datatypes.push_back(syncer::THEMES);
    datatypes.push_back(syncer::SUPERVISED_USERS);
    datatypes.push_back(syncer::SUPERVISED_USER_SHARED_SETTINGS);
#endif  // !OPERA_SYNC
#endif  // !OS_ANDROID

    // Common types.
#if !defined(OPERA_SYNC)
    datatypes.push_back(syncer::AUTOFILL);
    datatypes.push_back(syncer::AUTOFILL_PROFILE);
    datatypes.push_back(syncer::AUTOFILL_WALLET_DATA);
    datatypes.push_back(syncer::AUTOFILL_WALLET_METADATA);
#endif  // !OPERA_SYNC
    datatypes.push_back(syncer::BOOKMARKS);
    datatypes.push_back(syncer::DEVICE_INFO);
#if !defined(OPERA_SYNC)
    datatypes.push_back(syncer::FAVICON_TRACKING);
    datatypes.push_back(syncer::FAVICON_IMAGES);
#endif  // !OPERA_SYNC
    datatypes.push_back(syncer::HISTORY_DELETE_DIRECTIVES);
    datatypes.push_back(syncer::PASSWORDS);
    datatypes.push_back(syncer::PREFERENCES);
#if !defined(OPERA_SYNC)
    datatypes.push_back(syncer::PRIORITY_PREFERENCES);
#endif  // !OPERA_SYNC
    datatypes.push_back(syncer::SESSIONS);
    datatypes.push_back(syncer::PROXY_TABS);
#if !defined(OPERA_SYNC)
    datatypes.push_back(syncer::SUPERVISED_USER_SETTINGS);
    datatypes.push_back(syncer::SUPERVISED_USER_WHITELISTS);
#endif  // !OPERA_SYNC
    datatypes.push_back(syncer::TYPED_URLS);

    return datatypes;
  }

  // Returns the number of default datatypes.
  size_t DefaultDatatypesCount() { return DefaultDatatypes().size(); }

  // Asserts that all the default datatypes are in |map|, except
  // for |exception_type|, which unless it is UNDEFINED, is asserted to
  // not be in |map|.
  void CheckDefaultDatatypesInMapExcept(DataTypeController::StateMap* map,
                                        syncer::ModelTypeSet exception_types) {
    std::vector<syncer::ModelType> defaults = DefaultDatatypes();
    std::vector<syncer::ModelType>::iterator iter;
    for (iter = defaults.begin(); iter != defaults.end(); ++iter) {
      if (exception_types.Has(*iter))
        EXPECT_EQ(0U, map->count(*iter))
            << *iter << " found in dataypes map, shouldn't be there.";
      else
        EXPECT_EQ(1U, map->count(*iter)) << *iter
                                         << " not found in datatypes map";
    }
  }

  void SetDisabledTypes(syncer::ModelTypeSet disabled_types) {
    base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        switches::kDisableSyncTypes,
        syncer::ModelTypeSetToString(disabled_types));
  }

  Profile* profile() { return profile_.get(); }

 private:
#if defined(OPERA_SYNC)
  opera::ProfileSyncServiceParamsProviderProviderMock
      opera_pss_params_provider_mock_;
#endif  // OPERA_SYNC
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<Profile> profile_;
};

#if !defined(OPERA_SYNC)
// Verify that the disable sync flag disables creation of the sync service.
TEST_F(ProfileSyncServiceFactoryTest, DisableSyncFlag) {
  base::CommandLine::ForCurrentProcess()->AppendSwitch(switches::kDisableSync);
  EXPECT_EQ(nullptr, ProfileSyncServiceFactory::GetForProfile(profile()));
}
#endif  // !OPERA_SYNC

// Verify that a normal (no command line flags) PSS can be created and
// properly initialized.
TEST_F(ProfileSyncServiceFactoryTest, CreatePSSDefault) {
  ProfileSyncService* pss = ProfileSyncServiceFactory::GetForProfile(profile());
  DataTypeController::StateMap controller_states;
  pss->GetDataTypeControllerStates(&controller_states);
  EXPECT_EQ(DefaultDatatypesCount(), controller_states.size());
  CheckDefaultDatatypesInMapExcept(&controller_states, syncer::ModelTypeSet());
}

// Verify that a PSS with a disabled datatype can be created and properly
// initialized.
TEST_F(ProfileSyncServiceFactoryTest, CreatePSSDisableOne) {
  syncer::ModelTypeSet disabled_types(syncer::AUTOFILL);

#if defined(OPERA_SYNC)
  disabled_types.Remove(syncer::AUTOFILL);
  disabled_types.Put(syncer::PREFERENCES);
#endif  // OPERA_SYNC

  SetDisabledTypes(disabled_types);
  ProfileSyncService* pss = ProfileSyncServiceFactory::GetForProfile(profile());
  DataTypeController::StateMap controller_states;
  pss->GetDataTypeControllerStates(&controller_states);
  EXPECT_EQ(DefaultDatatypesCount() - disabled_types.Size(),
            controller_states.size());
  CheckDefaultDatatypesInMapExcept(&controller_states, disabled_types);
}

// Verify that a PSS with multiple disabled datatypes can be created and
// properly initialized.
TEST_F(ProfileSyncServiceFactoryTest, CreatePSSDisableMultiple) {
  syncer::ModelTypeSet disabled_types(syncer::AUTOFILL_PROFILE,
                                      syncer::BOOKMARKS);
#if defined(OPERA_SYNC)
  disabled_types.Remove(syncer::AUTOFILL_PROFILE);
  disabled_types.Put(syncer::PREFERENCES);
#endif  // OPERA_SYNC

  SetDisabledTypes(disabled_types);
  ProfileSyncService* pss = ProfileSyncServiceFactory::GetForProfile(profile());
  DataTypeController::StateMap controller_states;
  pss->GetDataTypeControllerStates(&controller_states);
  EXPECT_EQ(DefaultDatatypesCount() - disabled_types.Size(),
            controller_states.size());
  CheckDefaultDatatypesInMapExcept(&controller_states, disabled_types);
}
