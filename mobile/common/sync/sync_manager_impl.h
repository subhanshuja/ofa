// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_SYNC_SYNC_MANAGER_IMPL_H_
#define MOBILE_COMMON_SYNC_SYNC_MANAGER_IMPL_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/files/file_path.h"

#include "common/sync/profile_sync_service_params_provider.h"
#include "common/sync/sync_observer.h"
#include "common/sync/sync_types.h"

#include "mobile/common/sync/invalidation_data.h"
#include "mobile/common/sync/sync_manager.h"

#include "net/url_request/url_fetcher_delegate.h"

class Profile;

namespace bookmarks {
class BookmarkModel;
}  // namespace bookmarks

namespace browser_sync {
class ProfileSyncService;
}

namespace base {
class SingleThreadTaskRunner;
}

namespace content {
class BrowserContext;
class NotificationService;
}

namespace net {
class URLFetcher;
}

namespace opera {
struct AccountAuthData;
class AccountAuthDataFetcher;
}

namespace mobile {

class AuthData;
class SyncCredentialsStore;
class SyncLoginRequest;
class SyncLoginRetry;
class SyncProfile;
class SyncRegisterRetry;
class SyncedWindowDelegate;

class SyncManagerImpl
    : public SyncManager,
      public opera::SyncObserver,
      public InvalidationData::Observer,
      public net::URLFetcherDelegate,
      public opera::ProfileSyncServiceParamsProvider::Provider {
 public:
  SyncManagerImpl(const base::FilePath& base_path,
                  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
                  scoped_refptr<base::SequencedTaskRunner> io_task_runner,
                  content::BrowserContext* browser_context,
                  InvalidationData* invalidation_data,
                  AuthData* auth_data);
  virtual ~SyncManagerImpl();

  void AuthSetup() override;

  // ProfileSyncServiceParamsProvider overrides:
  void GetProfileSyncServiceInitParamsForProfile(
      Profile* profile,
      opera::OperaSyncInitParams* params) override;

  // mobile::SyncManager overrides:
  bookmarks::BookmarkModel* GetBookmarkModel() override;
  void InvalidateAll() override;
  void Logout() override;
  Status GetStatus() override;
  std::string GetDisplayName() override;

  Profile* GetProfile() override;

  void Flush() override;
  void Shutdown() override;

  void AddObserver(SyncManager::Observer* observer) override;
  void RemoveObserver(SyncManager::Observer* observer) override;

  void StartSessionRestore() override;
  void FinishSessionRestore() override;
  void InsertTab(int index, const SyncedTabData* data) override;
  void RemoveTab(int index) override;
  void UpdateTab(int index, const SyncedTabData* data) override;
  void SetActiveTab(int index) override;

  std::unique_ptr<syncer::DeviceInfo> GetDeviceInfoForName(
      const std::string& name) const override;

  // SyncObserver
  void OnSyncStatusChanged(syncer::SyncService* sync_service) override;

  // InvalidationData::Observer
  void InvalidationDataLoaded(InvalidationData* invalidation_data) override;
  void InvalidationDataChanged(InvalidationData* invalidation_data) override;

  // URLFetcherDelegate
  void OnURLFetchComplete(const net::URLFetcher* source) override;

 protected:
  friend class SyncRegisterRetry;
  friend class SyncLoginRetry;
  friend class SyncLoginRequest;

  browser_sync::ProfileSyncService* GetSyncService() const;

  void ReportLoginError();

 private:
  SyncProfile* GetSyncProfile() const;

  void RegisterDevice();
  void RetryRegisterDeviceAfterDelay();
  bool TryToLogin(bool at_startup);
  void RetryLoginAfterDelay();

  void ShowLoginPrompt(opera::SyncLoginUIParams params);

  void ShowConnectionStatus(
      int status,
      Profile* profile,
      const opera::ShowSyncLoginPromptCallback& prompt_callback);

  void UpdateStatus(Status status);

  static bool IsLoginErrorStatus(Status status);

  opera::AccountAuthDataFetcher* AuthDataFetcherFactory(
      const opera::AccountAuthData& old_data);

  void InitializeFavorites(const std::string& session_name);

  void RemoveRemoteSessions();

  InvalidationData* invalidation_data_;
  AuthData* auth_data_;
  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;
  std::unique_ptr<content::NotificationService> notification_service_;
  base::ObserverList<SyncManager::Observer> observer_list_;
  Status status_;
  Status reported_status_;
  std::unique_ptr<SyncLoginRequest> login_request_;
  std::string device_id_;
  std::unique_ptr<net::URLFetcher> register_device_fetcher_;
  scoped_refptr<SyncRegisterRetry> register_device_retry_;
  std::unique_ptr<SyncCredentialsStore> credentials_store_;
  std::unique_ptr<SyncedWindowDelegate> synced_window_delegate_;
  scoped_refptr<SyncLoginRetry> login_retry_;
  base::Time last_login_retry_;
  size_t next_login_retry_index_;

  DISALLOW_COPY_AND_ASSIGN(SyncManagerImpl);
};

}  // namespace mobile

#endif  // MOBILE_COMMON_SYNC_SYNC_MANAGER_IMPL_H_
