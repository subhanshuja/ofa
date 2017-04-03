// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_CHROME_SYNC_CLIENT_H__
#define CHROME_BROWSER_SYNC_CHROME_SYNC_CLIENT_H__

#include <memory>
#include <vector>

#include "base/macros.h"
#include "chrome/browser/sync/glue/extensions_activity_monitor.h"
#include "components/sync/driver/sync_client.h"

namespace autofill {
class AutofillWebDataService;
}
#if defined(OPERA_SYNC)
#include "common/sync/sync_account.h"
#include "common/sync/sync_error_handler.h"
#endif  // OPERA_SYNC

namespace password_manager {
class PasswordStore;
}
class Profile;

namespace syncer {
class DeviceInfoTracker;
class SyncApiComponentFactory;
class SyncService;
}

namespace browser_sync {

class ChromeSyncClient : public syncer::SyncClient {
 public:
  explicit ChromeSyncClient(Profile* profile);
  ~ChromeSyncClient() override;

  // SyncClient implementation.
  void Initialize() override;
  syncer::SyncService* GetSyncService() override;
  PrefService* GetPrefService() override;
  bookmarks::BookmarkModel* GetBookmarkModel() override;
  favicon::FaviconService* GetFaviconService() override;
  history::HistoryService* GetHistoryService() override;
#if defined(OPERA_SYNC)
  opera::DuplicateTrackerInterface* GetBookmarkDuplicateTracker() override;
  opera::SyncAccount* CreateSyncAccount(
      std::unique_ptr<opera::SyncLoginDataStore> login_data_store,
      opera::SyncAccount::AuthDataUpdaterFactory auth_data_updater_factory,
      opera::TimeSkewResolverFactory time_skew_resolver_factory,
      const base::Closure& stop_syncing_callback) override;
  opera::SyncAccount* GetSyncAccount() override;
  opera::SyncErrorHandler* CreateSyncErrorHandler(
      base::TimeDelta show_status_delay,
      std::unique_ptr<opera::SyncDelayProvider> delay_provider,
      const opera::ShowSyncConnectionStatusCallback& show_status_callback,
      const opera::ShowSyncLoginPromptCallback& show_login_prompt_callback)
      override;
  opera::oauth2::AuthService* GetOperaOAuth2Service() override;
  opera::sync::SyncAuthKeeper* GetOperaSyncAuthKeeper() override;
  void OnSyncAuthKeeperInitialized(
      opera::sync::SyncAuthKeeper* keeper) override;
#if defined(OPERA_DESKTOP)
  LoginUIService::LoginUI* GetLoginUI() override;
  opera::OtherSpeedDialsCleaner* GetOtherSpeeddialsCleaner() override;
  scoped_refptr<opera::SyncPasswordRecoverer>
      CreateOperaSyncPasswordRecoverer() override;
#endif  // OPERA_DESKTOP
#endif  // OPERA_SYNC
  base::Closure GetPasswordStateChangedCallback() override;
  syncer::SyncApiComponentFactory::RegisterDataTypesMethod
  GetRegisterPlatformTypesCallback() override;
  autofill::PersonalDataManager* GetPersonalDataManager() override;
  invalidation::InvalidationService* GetInvalidationService() override;
  BookmarkUndoService* GetBookmarkUndoServiceIfExists() override;
  scoped_refptr<syncer::ExtensionsActivity> GetExtensionsActivity() override;
  sync_sessions::SyncSessionsClient* GetSyncSessionsClient() override;
  base::WeakPtr<syncer::SyncableService> GetSyncableServiceForType(
      syncer::ModelType type) override;
  base::WeakPtr<syncer::ModelTypeService> GetModelTypeServiceForType(
      syncer::ModelType type) override;
  scoped_refptr<syncer::ModelSafeWorker> CreateModelWorkerForGroup(
      syncer::ModelSafeGroup group,
      syncer::WorkerLoopDestructionObserver* observer) override;
  syncer::SyncApiComponentFactory* GetSyncApiComponentFactory() override;

  // Helpers for overriding getters in tests.
  void SetSyncApiComponentFactoryForTesting(
      std::unique_ptr<syncer::SyncApiComponentFactory> component_factory);

  // Iterates over all of the profiles that have been loaded so far, and
  // extracts their tracker if present. If some profiles don't have trackers, no
  // indication is given in the passed vector.
  static void GetDeviceInfoTrackers(
      std::vector<const syncer::DeviceInfoTracker*>* trackers);

 private:
  // Register data types which are enabled on desktop platforms only.
  // |disabled_types| and |enabled_types| correspond only to those types
  // being explicitly disabled/enabled by the command line.
  void RegisterDesktopDataTypes(syncer::SyncService* sync_service,
                                syncer::ModelTypeSet disabled_types,
                                syncer::ModelTypeSet enabled_types);

  // Register data types which are enabled on Android platforms only.
  // |disabled_types| and |enabled_types| correspond only to those types
  // being explicitly disabled/enabled by the command line.
  void RegisterAndroidDataTypes(syncer::SyncService* sync_service,
                                syncer::ModelTypeSet disabled_types,
                                syncer::ModelTypeSet enabled_types);

  Profile* const profile_;

  // The sync api component factory in use by this client.
  std::unique_ptr<syncer::SyncApiComponentFactory> component_factory_;

  // Members that must be fetched on the UI thread but accessed on their
  // respective backend threads.
  scoped_refptr<autofill::AutofillWebDataService> web_data_service_;
  scoped_refptr<password_manager::PasswordStore> password_store_;

  std::unique_ptr<sync_sessions::SyncSessionsClient> sync_sessions_client_;

  // Generates and monitors the ExtensionsActivity object used by sync.
  ExtensionsActivityMonitor extensions_activity_monitor_;

#if defined(OPERA_SYNC)
  std::unique_ptr<opera::SyncAccount> account_;
#if defined(OPERA_DESKTOP)
  std::unique_ptr<opera::OtherSpeedDialsCleaner> other_speed_dials_cleaner_;
#endif  // OPERA_DESKTOP
#endif  // OPERA_SYNC

  base::WeakPtrFactory<ChromeSyncClient> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ChromeSyncClient);
};

}  // namespace browser_sync

#endif  // CHROME_BROWSER_SYNC_CHROME_SYNC_CLIENT_H__
