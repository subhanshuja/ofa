// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sync_error_handler.h"

#include "base/bind.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/engine/cycle/sync_cycle_snapshot.h"
#include "net/base/net_errors.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "common/sync/sync_delay_provider.h"
#include "common/sync/sync_server_info.h"
#include "common/sync/sync_status.h"

namespace opera {
namespace {

class OperaFakeSyncService : public syncer::SyncService {
 public:
  OperaFakeSyncService()
      : sync_service_url_("http://fake.sync.server.com/"),
        auth_error_(GoogleServiceAuthError(GoogleServiceAuthError::NONE)),
        sync_status_(net::OK,
                     SyncServerInfo(),
                     false,
                     true,
                     false,
                     "",
                     false,
                     false,
                     false,
                     syncer::ModelTypeSet()) {}
  ~OperaFakeSyncService() override {}

  void set_opera_sync_status(opera::SyncStatus sync_status) {
    sync_status_ = sync_status;
  }

  // sync_driver::DataTypeEncryptionHandler
  bool IsPassphraseRequired() const override { return false; }
  syncer::ModelTypeSet GetEncryptedDataTypes() const override {
    return syncer::ModelTypeSet();
  }

  // sync_driver::SyncService
  bool IsFirstSetupComplete() const override { return false; }
  bool IsSyncAllowed() const override { return false; }
  bool IsSyncActive() const override { return false; }
  void TriggerRefresh(const syncer::ModelTypeSet& types) override {}
  syncer::ModelTypeSet GetActiveDataTypes() const override {
    return syncer::ModelTypeSet();
  }
  syncer::SyncClient* GetSyncClient() const override { return nullptr; }
  void AddObserver(syncer::SyncServiceObserver* observer) override {}
  void RemoveObserver(syncer::SyncServiceObserver* observer) override {}
#if defined(OPERA_SYNC)
  void AddSyncStateObserver(opera::SyncObserver* observer) override {}
  void RemoveSyncStateObserver(opera::SyncObserver* observer) override {}
#endif  // OPERA_SYNC
  bool HasObserver(
      const syncer::SyncServiceObserver* observer) const override {
    return false;
  }
  void OnDataTypeRequestsSyncStartup(syncer::ModelType type) override {}
  bool CanSyncStart() const override { return false; }
  void RequestStop(SyncStopDataFate data_fate) override {}
  void RequestStart() override {}
  syncer::ModelTypeSet GetPreferredDataTypes() const override {
    return syncer::ModelTypeSet();
  }
  void OnUserChoseDatatypes(bool sync_everything,
                            syncer::ModelTypeSet chosen_types) override {}
  void SetFirstSetupComplete() override {}
  bool IsFirstSetupInProgress() const override { return false; }
  std::unique_ptr<syncer::SyncSetupInProgressHandle>
      GetSetupInProgressHandle() override { return nullptr; }
  bool IsSetupInProgress() const override { return false; }
  bool ConfigurationDone() const override { return false; }
  const GoogleServiceAuthError& GetAuthError() const override {
    return auth_error_;
  }
  bool HasUnrecoverableError() const override { return false; }
  bool IsBackendInitialized() const override { return false; }
  sync_sessions::OpenTabsUIDelegate* GetOpenTabsUIDelegate() override {
    return nullptr;
  }
  bool IsPassphraseRequiredForDecryption() const override { return false; }
  base::Time GetExplicitPassphraseTime() const override { return base::Time(); }
  bool IsUsingSecondaryPassphrase() const override { return false; }
  void EnableEncryptEverything() override {}
  bool IsEncryptEverythingEnabled() const override { return false; }
  void SetEncryptionPassphrase(const std::string& passphrase,
                               PassphraseType type) override {}
  bool SetDecryptionPassphrase(const std::string& passphrase)
      override {
    return false;
  }
  bool IsCryptographerReady(
      const syncer::BaseTransaction* trans) const override {
    return false;
  }
  syncer::UserShare* GetUserShare() const override { return nullptr; }
  syncer::LocalDeviceInfoProvider* GetLocalDeviceInfoProvider()
      const override {
    return nullptr;
  }
  void RegisterDataTypeController(
      std::unique_ptr<syncer::DataTypeController> data_type_controller) override {}
  void ReenableDatatype(syncer::ModelType type) override {}
  SyncService::SyncTokenStatus GetSyncTokenStatus() const override {
    return SyncTokenStatus();
  }
  std::string QuerySyncStatusSummaryString() override { return ""; }
  bool QueryDetailedSyncStatus(syncer::SyncStatus* result) override {
    result = nullptr;
    return false;
  }
  base::string16 GetLastSyncedTimeString() const override {
    return base::string16();
  }
  std::string GetBackendInitializationStateString() const override {
    return std::string();
  }
  syncer::SyncCycleSnapshot GetLastCycleSnapshot()
      const override {
    return syncer::SyncCycleSnapshot();
  }
  base::Value* GetTypeStatusMap() const override { return nullptr; }
  const GURL& sync_service_url() const override { return sync_service_url_; }
  std::string unrecoverable_error_message() const override {
    return std::string();
  }
  tracked_objects::Location unrecoverable_error_location() const override {
    return FROM_HERE;
  }
  void AddProtocolEventObserver(
      syncer::ProtocolEventObserver* observer) override {}
  void RemoveProtocolEventObserver(
      syncer::ProtocolEventObserver* observer) override {}
  void AddTypeDebugInfoObserver(
      syncer::TypeDebugInfoObserver* observer) override {}
  void RemoveTypeDebugInfoObserver(
      syncer::TypeDebugInfoObserver* observer) override {}
  base::WeakPtr<syncer::JsController> GetJsController() override {
    return base::WeakPtr<syncer::JsController>();
  }
  void GetAllNodes(const base::Callback<void(std::unique_ptr<base::ListValue>)>&
                       callback) override {}
#if defined(OPERA_SYNC)
  opera::SyncAccount* account() const override { return nullptr; }
  opera::SyncStatus opera_sync_status() override { return sync_status_; }
  void OnSyncAuthKeeperInitialized() override {}
#endif  // OPERA_SYNC

  GURL sync_service_url_;
  GoogleServiceAuthError auth_error_;
  opera::SyncStatus sync_status_;
};

class FakeStatusProvider {
 public:
  explicit FakeStatusProvider(SyncObserver* observer)
      : sync_service_(new OperaFakeSyncService()), observer_(observer) {}

  SyncStatus GetStatus() const { return sync_service_->opera_sync_status(); }

  void Change(int network_error,
              const SyncServerInfo& info,
              bool busy,
              bool logged_in,
              bool internal_error,
              std::string internal_error_message) {
    const SyncStatus new_status = SyncStatus(
        network_error, info, busy, logged_in, internal_error,
        internal_error_message, false, false, false, syncer::ModelTypeSet());
    sync_service_->set_opera_sync_status(new_status);
    observer_->OnSyncStatusChanged(sync_service_);
  }

 private:
  OperaFakeSyncService* sync_service_;
  SyncObserver* observer_;
};

class FakeSyncDelayProvider : public SyncDelayProvider {
 public:
  /**
   * @param last_callback the pointee is set to the callback passed to
   *    InvokeAfter() on each call, and reset on each call to Cancel().  This
   *    parameter can be used to verify InvokeAfter()/Cancel() is called.
   *    Invoking the callback after a call to InvokeAfter() fakes the lapse of
   *    the delay.
   */
  explicit FakeSyncDelayProvider(base::Closure* last_callback)
      : last_callback_(last_callback) {}

  /**
   * @name SyncDelayProvider implementation
   * @{
   */
  void InvokeAfter(base::TimeDelta delay,
                   const base::Closure& callback) override {
    if (last_callback_ != NULL)
      *last_callback_ = callback;
  }
  bool IsScheduled() override { return !last_callback_->is_null(); }
  void Cancel() override {
    if (last_callback_ != NULL)
      last_callback_->Reset();
  }
  /** @} */

 private:
  base::Closure* const last_callback_;
};

class SyncErrorHandlerTest : public ::testing::Test {
 public:
  SyncErrorHandlerTest()
      : handler_(nullptr,
                 base::TimeDelta::FromSeconds(111),
                 std::unique_ptr<SyncDelayProvider>(
                     new FakeSyncDelayProvider(&timer_callback_)),
                 base::Bind(&SyncErrorHandlerTest::ShowStatus,
                            base::Unretained(this)),
                 ShowSyncLoginPromptCallback()),
        status_(&handler_),
        connection_lost_(false),
        connection_restored_(false),
        connection_server_error_(false) {
    enabled_info_.server_status = SyncServerInfo::SYNC_OK;
    error_info_.server_status = SyncServerInfo::SYNC_UNKNOWN_ERROR;
  }

 protected:
  base::Closure timer_callback_;
  SyncErrorHandler handler_;
  FakeStatusProvider status_;
  bool connection_lost_;
  bool connection_restored_;
  bool connection_server_error_;
  SyncServerInfo disabled_info_;
  SyncServerInfo enabled_info_;
  SyncServerInfo error_info_;

 private:
  void ShowStatus(int status,
                  Profile* /* profile */,
                  const ShowSyncLoginPromptCallback& /* show_login_prompt */) {
    switch (status) {
      case SYNC_CONNECTION_LOST:
        connection_lost_ = true;
        break;
      case SYNC_CONNECTION_RESTORED:
        connection_restored_ = true;
        break;
      case SYNC_CONNECTION_SERVER_ERROR:
        connection_server_error_ = true;
        break;
      default:
        NOTREACHED();
        break;
    }
  }
};

TEST_F(SyncErrorHandlerTest, IgnoresUninterestingChanges) {
  status_.Change(net::OK, disabled_info_, true, false, false, "");
  status_.Change(net::OK, enabled_info_, false, true, false, "");
  status_.Change(net::OK, enabled_info_, true, true, false, "");

  EXPECT_TRUE(timer_callback_.is_null());
  EXPECT_FALSE(connection_lost_);
  EXPECT_FALSE(connection_restored_);
  EXPECT_FALSE(connection_server_error_);
}

TEST_F(SyncErrorHandlerTest, ShowsConnectionLostStatusLoggingIn) {
  status_.Change(net::OK, disabled_info_, true, false, false, "");
  status_.Change(net::ERR_TIMED_OUT, disabled_info_, true, false, false, "");

  ASSERT_FALSE(timer_callback_.is_null());
  timer_callback_.Run();

  EXPECT_TRUE(connection_lost_);
  EXPECT_FALSE(connection_restored_);
  EXPECT_FALSE(connection_server_error_);
}

TEST_F(SyncErrorHandlerTest, ShowsConnectionLostStatusSyncing) {
  status_.Change(net::OK, disabled_info_, true, false, false, "");
  status_.Change(net::OK, enabled_info_, false, true, false, "");
  status_.Change(net::OK, enabled_info_, true, true, false, "");
  status_.Change(net::ERR_TIMED_OUT, enabled_info_, true, true, false, "");

  ASSERT_FALSE(timer_callback_.is_null());
  timer_callback_.Run();

  EXPECT_TRUE(connection_lost_);
  EXPECT_FALSE(connection_restored_);
  EXPECT_FALSE(connection_server_error_);
}

TEST_F(SyncErrorHandlerTest, ShowsConnectionRestoredLoggingIn) {
  status_.Change(net::OK, disabled_info_, true, false, false, "");
  status_.Change(net::ERR_TIMED_OUT, disabled_info_, true, false, false, "");

  ASSERT_FALSE(timer_callback_.is_null());
  timer_callback_.Run();

  status_.Change(net::OK, enabled_info_, false, true, false, "");

  EXPECT_TRUE(connection_restored_);
}

TEST_F(SyncErrorHandlerTest, ShowsConnectionRestoredSyncing) {
  status_.Change(net::OK, enabled_info_, true, true, false, "");
  status_.Change(net::ERR_TIMED_OUT, enabled_info_, true, true, false, "");

  ASSERT_FALSE(timer_callback_.is_null());
  timer_callback_.Run();

  status_.Change(net::OK, enabled_info_, false, true, false, "");

  EXPECT_TRUE(connection_restored_);
}

TEST_F(SyncErrorHandlerTest, IgnoresFastInterestingChanges) {
  // Logging in
  status_.Change(net::OK, disabled_info_, true, false, false, "");
  status_.Change(net::ERR_TIMED_OUT, disabled_info_, true, false, false, "");
  status_.Change(net::OK, enabled_info_, false, true, false, "");

  EXPECT_TRUE(timer_callback_.is_null());
  EXPECT_FALSE(connection_lost_);
  EXPECT_FALSE(connection_restored_);

  // Syncing
  status_.Change(net::OK, enabled_info_, true, true, false, "");
  status_.Change(net::ERR_TIMED_OUT, enabled_info_, true, true, false, "");
  status_.Change(net::OK, enabled_info_, false, true, false, "");

  EXPECT_TRUE(timer_callback_.is_null());
  EXPECT_FALSE(connection_lost_);
  EXPECT_FALSE(connection_restored_);

  // An unignored interesting change
  status_.Change(net::OK, enabled_info_, true, true, false, "");
  status_.Change(net::ERR_TIMED_OUT, enabled_info_, true, true, false, "");

  ASSERT_FALSE(timer_callback_.is_null());
  timer_callback_.Run();

  status_.Change(net::OK, enabled_info_, false, true, false, "");

  // Syncing again
  connection_lost_ = false;
  connection_restored_ = false;
  status_.Change(net::OK, enabled_info_, true, true, false, "");
  status_.Change(net::ERR_TIMED_OUT, enabled_info_, true, true, false, "");
  status_.Change(net::OK, enabled_info_, false, true, false, "");

  EXPECT_TRUE(timer_callback_.is_null());
  EXPECT_FALSE(connection_lost_);
  EXPECT_FALSE(connection_restored_);
}

TEST_F(SyncErrorHandlerTest, ServerErrorMeansConnectionNotRestored) {
  status_.Change(net::OK, enabled_info_, true, true, false, "");
  status_.Change(net::ERR_TIMED_OUT, enabled_info_, true, true, false, "");

  EXPECT_FALSE(timer_callback_.is_null());

  status_.Change(net::OK, error_info_, false, true, false, "");

  EXPECT_TRUE(timer_callback_.is_null());
  EXPECT_FALSE(connection_restored_);

  // Must still show "connection lost" the next time it happens.
  status_.Change(net::OK, error_info_, true, true, false, "");
  status_.Change(net::ERR_TIMED_OUT, error_info_, true, true, false, "");

  ASSERT_FALSE(timer_callback_.is_null());
  timer_callback_.Run();

  EXPECT_TRUE(connection_lost_);
  EXPECT_FALSE(connection_restored_);
}

TEST_F(SyncErrorHandlerTest, LogOutMeansConnectionNotRestored) {
  status_.Change(net::OK, enabled_info_, true, true, false, "");
  status_.Change(net::ERR_TIMED_OUT, enabled_info_, true, true, false, "");

  EXPECT_FALSE(timer_callback_.is_null());

  status_.Change(net::OK, disabled_info_, false, false, false, "");

  EXPECT_TRUE(timer_callback_.is_null());
  EXPECT_FALSE(connection_restored_);
}

TEST_F(SyncErrorHandlerTest, ShowsServerError) {
  status_.Change(net::OK, enabled_info_, true, true, false, "");
  status_.Change(net::OK, error_info_, false, true, false, "");

  EXPECT_FALSE(connection_lost_);
  EXPECT_FALSE(connection_restored_);
  EXPECT_TRUE(connection_server_error_);
}

TEST_F(SyncErrorHandlerTest, IgnoresServerErrorIfNeverEnabled) {
  status_.Change(net::OK, disabled_info_, true, false, false, "");
  status_.Change(net::OK, error_info_, false, true, false, "");

  EXPECT_FALSE(connection_server_error_);
}

TEST_F(SyncErrorHandlerTest, DisconnectedOnInternalError) {
  status_.Change(net::OK, enabled_info_, true, true, false, "");
  status_.Change(net::OK, enabled_info_, true, true, true, "test error");

  EXPECT_FALSE(connection_lost_);
  EXPECT_FALSE(connection_restored_);
  EXPECT_FALSE(connection_server_error_);
}

}  // namespace
}  // namespace opera
