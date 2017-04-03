// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "mobile/common/sync/sync_manager_impl.h"

#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/ptr_util.h"
#include "base/rand_util.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"

#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/bookmarks/startup_task_runner_service_factory.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/common/pref_names.h"

#include "common/account/account_service.h"
#include "common/account/account_util.h"
#include "common/account/time_skew_resolver_impl.h"
#include "common/constants/pref_names.h"
#include "common/sync/sync_auth_data_updater.h"
#include "common/sync/sync_config.h"
#include "common/sync/sync_login_data.h"
#include "common/sync/sync_login_data_store_impl.h"
#include "common/sync/sync_login_error_data.h"

#include "components/bookmarks/browser/startup_task_runner_service.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/invalidation/impl/invalidation_service_android.h"
#include "components/invalidation/impl/invalidator_storage.h"
#include "components/sync/device_info/device_info_tracker.h"
#include "components/sync_sessions/open_tabs_ui_delegate.h"
#include "components/sync_sessions/synced_session.h"

#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"

#include "mobile/common/favorites/favorites.h"
#include "mobile/common/sync/auth_data.h"
#include "mobile/common/sync/sync_browser_process.h"
#include "mobile/common/sync/sync_credentials_store.h"
#include "mobile/common/sync/sync_profile.h"
#include "mobile/common/sync/sync_profile_manager.h"
#include "mobile/common/sync/synced_window_delegate.h"

#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_status.h"

#include "components/sync/engine_impl/net/url_translator.h"
#include "components/sync/base/get_session_name.h"

namespace mobile {

namespace {

const int64_t kRegisterDeviceRetryDelayInSec = 2 * 60;

const int64_t kLoginRetryDelayInMilliSec[] = {500, 5000, 35000};
// Number of ranges in kLoginRetryDelayInMilliSec, not the number of items
const size_t kMaxLoginRetries = 2;

std::string ServiceTypeToString(SyncManager::ServiceType service_type) {
  switch (service_type) {
    case SyncManager::SERVICE_GCM:
      return "gcm";
    case SyncManager::SERVICE_APN:
      return "apn";
  }
  NOTREACHED();
  return "error";
}

std::string BuildToString(SyncManager::Build build) {
  switch (build) {
    case SyncManager::BUILD_DEVELOPER:
      return "dev";
    case SyncManager::BUILD_INTERNAL:
      return "int";
    case SyncManager::BUILD_PUBLIC:
      return "pub";
  }
  NOTREACHED();
  return "dev";
}

}  // namespace

class SyncLoginRequest : public AuthData::Observer {
 public:
  SyncLoginRequest(AuthData* auth_data,
                   SyncManagerImpl* manager,
                   SyncCredentialsStore* store)
      : auth_data_(auth_data),
        manager_(manager),
        store_(store),
        token_requested_(false) {
    auth_data_->AddObserver(this);
  }

  ~SyncLoginRequest() {
    base::STLClearObject(&password_);
    auth_data_->RemoveObserver(this);
  }

  bool Active() const {
    return token_requested_ || login_data_.has_auth_data();
  }

  void AuthDataLoggedIn(const std::string& login_name,
                        const std::string& user_name,
                        const std::string& password) override {
    login_name_ = login_name;
    password_ = password;
    if (login_name != user_name) {
      login_data_.user_name = user_name;
      login_data_.user_email = login_name;
      login_data_.used_username_to_login = false;
    } else {
      login_data_.user_name = user_name;
      login_data_.user_email = "";
      login_data_.used_username_to_login = true;
    }
    ResetAuthData();
    token_requested_ = true;
    auth_data_->RequestToken(login_name, password);
  }

  void AuthDataGotToken(const std::string& token,
                        const std::string& secret) override {
    DCHECK(token_requested_);
    token_requested_ = false;

    store_->Store(login_name_, login_data_.user_name, password_);
    login_name_.clear();
    base::STLClearObject(&password_);

    login_data_.auth_data.token = token;
    login_data_.auth_data.token_secret = secret;

    browser_sync::ProfileSyncService* sync_service = manager_->GetSyncService();
    sync_service->SetFirstSetupComplete();
    sync_service->account()->LogInWithAuthData(login_data_);
    ResetAuthData();
  }

  void AuthDataLogin(const std::string& token,
                     const std::string& secret,
                     const std::string& display_name) override {
    DCHECK(!token_requested_);
    login_data_.user_name = display_name;
    login_data_.user_email = "";
    login_data_.used_username_to_login = true;

    login_data_.auth_data.token = token;
    login_data_.auth_data.token_secret = secret;

    browser_sync::ProfileSyncService* sync_service = manager_->GetSyncService();
    sync_service->SetFirstSetupComplete();
    sync_service->account()->LogInWithAuthData(login_data_);
    ResetAuthData();
  }

  void AuthDataError(AuthData::Error error,
                     const std::string& message) override {
    if (!token_requested_)
      return;
    base::STLClearObject(&password_);
    token_requested_ = false;

    browser_sync::ProfileSyncService* sync_service = manager_->GetSyncService();

    opera::SyncLoginErrorData login_error;
    login_error.code = opera::SyncLoginErrorData::SYNC_LOGIN_UNKNOWN_ERROR;
    login_error.message = message;
    sync_service->SetLoginErrorData(login_error);
    sync_service->SetFirstSetupComplete();
    manager_->ReportLoginError();
  }

 private:
  void ResetAuthData() {
    login_data_.auth_data.token = "";
    login_data_.auth_data.token_secret = "";
  }

  AuthData* auth_data_;
  SyncManagerImpl* manager_;
  SyncCredentialsStore* store_;
  bool token_requested_;
  opera::SyncLoginData login_data_;
  std::string login_name_;
  std::string password_;
};

class SyncRegisterRetry : public base::RefCounted<SyncRegisterRetry> {
 public:
  explicit SyncRegisterRetry(SyncManagerImpl* sync_manager)
      : sync_manager_(sync_manager) {}

  void Retry() {
    if (sync_manager_) {
      sync_manager_->RegisterDevice();
    }
  }

  void Cancel() { sync_manager_ = NULL; }

 private:
  friend class base::RefCounted<SyncRegisterRetry>;
  ~SyncRegisterRetry() {}

  SyncManagerImpl* sync_manager_;
};

class SyncLoginRetry : public base::RefCounted<SyncLoginRetry> {
 public:
  explicit SyncLoginRetry(SyncManagerImpl* sync_manager)
      : sync_manager_(sync_manager) {}

  void Retry() {
    if (sync_manager_) {
      sync_manager_->TryToLogin(false);
    }
  }

  void Cancel() { sync_manager_ = NULL; }

 private:
  friend class base::RefCounted<SyncLoginRetry>;
  ~SyncLoginRetry() {}

  SyncManagerImpl* sync_manager_;
};

SyncManagerImpl::SyncManagerImpl(
    const base::FilePath& base_path,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    scoped_refptr<base::SequencedTaskRunner> io_task_runner,
    content::BrowserContext* browser_context,
    InvalidationData* invalidation_data,
    AuthData* auth_data)
    : invalidation_data_(invalidation_data),
      auth_data_(auth_data),
      ui_task_runner_(ui_task_runner),
      status_(STATUS_SETUP),
      reported_status_(STATUS_SETUP),
      last_login_retry_(base::Time::UnixEpoch()),
      next_login_retry_index_(0) {
  DCHECK(browser_context);
  DCHECK(invalidation_data && auth_data);

  {
    // Create profile proxy (around browser context)
    std::unique_ptr<SyncProfile> profile =
        base::MakeUnique<SyncProfile>(browser_context, io_task_runner);
    // Create fake browser process singleton
    new SyncBrowserProcess(std::move(profile), base_path, io_task_runner);
  }

  // Setup window delegate used by SESSION data type
  synced_window_delegate_.reset(new SyncedWindowDelegate(
      this, base_path.Append(FILE_PATH_LITERAL("synced_tabs.db"))));

  // Create storage for username + password data
  credentials_store_.reset(new SyncCredentialsStore(GetProfile()->GetPrefs()));

  if (!opera::SyncConfig::UsesOAuth2()) {
    // Create login request to be sent whenever user credentials change or we
    // need to renew the auth token
    login_request_.reset(
        new SyncLoginRequest(auth_data_, this, credentials_store_.get()));
  }

  // Invalidation data receives device pushes
  if (invalidation_data_->IsLoaded()) {
    device_id_ = invalidation_data_->GetDeviceId();
  }
  invalidation_data_->AddObserver(this);

  // Initialize favorites after sync have decided on a session name
  syncer::GetSessionName(
      content::BrowserThread::GetBlockingPool()
          ->GetTaskRunnerWithShutdownBehavior(
              base::SequencedWorkerPool::WorkerShutdown::SKIP_ON_SHUTDOWN),
      base::Bind(&SyncManagerImpl::InitializeFavorites,
                 base::Unretained(this)));

  // Setup profile sync service
  opera::ProfileSyncServiceParamsProvider::GetInstance()->set_provider(this);

  GetSyncService()->AddSyncStateObserver(this);

  // Start deferred sync services
  StartupTaskRunnerServiceFactory::GetForProfile(GetProfile())
      ->StartDeferredTaskRunners();

  if (!opera::SyncConfig::UsesOAuth2()) {
    // Check if we're missing a valid auth token, if so, try to login (get a new
    // token) with the stored credentials if any
    if (TryToLogin(true)) {
      status_ = STATUS_LOGIN_ERROR;
      UpdateStatus(status_);
    } else {
      ShowLoginPrompt(
          opera::SyncLoginUIParams::SYNC_LOGIN_UI_WITH_SUPPRESS_OPTION);
    }
  }

  // Force Bookmarks to be connected with sync *now* so that any local changes
  // wont cause merges before sync has logged in
  GetSyncService()->OnDataTypeRequestsSyncStartup(syncer::ModelType::BOOKMARKS);
}

SyncManagerImpl::~SyncManagerImpl() {
  if (register_device_retry_) {
    register_device_retry_->Cancel();
  }
  if (login_retry_) {
    login_retry_->Cancel();
  }
  GetSyncService()->RemoveSyncStateObserver(this);
  invalidation_data_->RemoveObserver(this);
}

void SyncManagerImpl::AuthSetup() {
  auth_data_->Setup(SYNC_AUTH_APPLICATION, SYNC_AUTH_APPLICATION_KEY,
                    opera::SyncConfig::ClientKey(),
                    opera::SyncConfig::ClientSecret());
}

void SyncManagerImpl::GetProfileSyncServiceInitParamsForProfile(
    Profile* profile,
    opera::OperaSyncInitParams* params) {
  params->show_login_prompt =
      base::Bind(&SyncManagerImpl::ShowLoginPrompt, base::Unretained(this));
  params->show_connection_status = base::Bind(
      &SyncManagerImpl::ShowConnectionStatus, base::Unretained(this));
  params->time_skew_resolver_factory =
      base::Bind(&opera::TimeSkewResolverImpl::Create,
                 base::Unretained(GetProfile()->GetRequestContext()),
                 opera::SyncConfig::AuthServerURL());

  if (!opera::SyncConfig::UsesOAuth2()) {
    params->login_data_store.reset(new opera::SyncLoginDataStoreImpl(
        profile->GetPrefs(), opera::prefs::kSyncLoginData));
    params->auth_data_updater_factory = base::Bind(
        &SyncManagerImpl::AuthDataFetcherFactory, base::Unretained(this));
  }
}

browser_sync::ProfileSyncService* SyncManagerImpl::GetSyncService() const {
  return ProfileSyncServiceFactory::GetForProfile(GetSyncProfile());
}

void SyncManagerImpl::InvalidationDataLoaded(
    InvalidationData* invalidation_data) {
  DCHECK_EQ(invalidation_data_, invalidation_data);
  device_id_ = invalidation_data_->GetDeviceId();
  RegisterDevice();
}

void SyncManagerImpl::InvalidationDataChanged(
    InvalidationData* invalidation_data) {
  DCHECK_EQ(invalidation_data_, invalidation_data);
  DCHECK(device_id_ != invalidation_data_->GetDeviceId());
  device_id_ = invalidation_data_->GetDeviceId();
  RegisterDevice();
}

bookmarks::BookmarkModel* SyncManagerImpl::GetBookmarkModel() {
  return BookmarkModelFactory::GetForBrowserContext(GetProfile());
}

void SyncManagerImpl::InvalidateAll() {
  if (!ui_task_runner_->BelongsToCurrentThread()) {
    ui_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&SyncManagerImpl::InvalidateAll, base::Unretained(this)));
    return;
  }
  auto service = GetSyncService()->sync_client()->GetInvalidationService();
  static_cast<invalidation::InvalidationServiceAndroid*>(service)->Invalidate(
      NULL, NULL, 0, NULL, 0, NULL);
}

SyncManager::Status SyncManagerImpl::GetStatus() {
  return status_;
}

void SyncManagerImpl::RegisterDevice() {
  if (opera::SyncConfig::UsesOAuth2())
    return;

  if (register_device_retry_) {
    register_device_retry_->Cancel();
    register_device_retry_ = NULL;
  }

  if (status_ != STATUS_RUNNING || device_id_.empty())
    return;

  if (!GetSyncService()->account()->IsLoggedIn())
    return;

  if (!GetSyncService()->account()->HasValidAuthData() ||
      !GetSyncService()->IsBackendInitialized()) {
    RetryRegisterDeviceAfterDelay();
    return;
  }

  if (register_device_fetcher_)
    return;

  auto client_id =
      GetSyncService()->GetLocalDeviceInfoProvider()->GetLocalSyncCacheGUID();
  GURL url(opera::SyncConfig::SyncServerURL().spec() + "/register_device/?" +
           syncer::MakeSyncQueryString(client_id));
  base::DictionaryValue data;
  data.SetString("service_type",
                 ServiceTypeToString(invalidation_data_->GetServiceType()));
  data.SetString("build", BuildToString(invalidation_data_->GetBuild()));
  data.SetString("device_id", device_id_);
  auto service = GetSyncService()->sync_client()->GetInvalidationService();
  data.SetString("invalidator_id", service->GetInvalidatorClientId());
  std::string post_data;
  base::JSONWriter::Write(data, &post_data);

  register_device_fetcher_ =
      net::URLFetcher::Create(url, net::URLFetcher::POST, this);
  register_device_fetcher_->SetRequestContext(
      GetProfile()->GetRequestContext());
  register_device_fetcher_->SetLoadFlags(net::LOAD_DISABLE_CACHE |
                                         net::LOAD_DO_NOT_SAVE_COOKIES |
                                         net::LOAD_DO_NOT_SEND_COOKIES);
  std::string headers =
      "Authorization: " +
      GetSyncService()->account()->service()->GetSignedAuthHeader(
          url, opera::AccountService::HTTP_METHOD_POST,
          opera::SyncConfig::AuthServerURL().host());
  VLOG(2) << "Device registration (" + device_id_ + "): " + headers;
  register_device_fetcher_->SetExtraRequestHeaders(headers);
  register_device_fetcher_->SetUploadData("application/json", post_data);
  register_device_fetcher_->Start();
}

void SyncManagerImpl::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK_EQ(source, register_device_fetcher_.get());

  std::unique_ptr<net::URLFetcher> fetcher(std::move(register_device_fetcher_));
  std::string error;

  switch (source->GetResponseCode()) {
    case net::URLFetcher::RESPONSE_CODE_INVALID: {
      net::URLRequestStatus status = source->GetStatus();
      switch (status.status()) {
        case net::URLRequestStatus::Status::CANCELED:
          return;
        case net::URLRequestStatus::Status::FAILED:
          error = net::ErrorToString(status.error());
          break;
        case net::URLRequestStatus::Status::IO_PENDING:
        case net::URLRequestStatus::Status::SUCCESS:
          error = "Invalid URLFetcher state";
          break;
      }
      break;
    }
    case 200:
      VLOG(2) << "Device registration succeeded";
      return;
    case 400: {
      std::string json_data;
      if (source->GetResponseAsString(&json_data)) {
        std::unique_ptr<base::Value> data(base::JSONReader::Read(json_data));
        base::DictionaryValue* dict;
        if (!data || !data->GetAsDictionary(&dict) ||
            !dict->GetString("detail", &error)) {
          error = "Invalid JSON data in response";
        }
      } else {
        DCHECK(false);
      }
      break;
    }
    default: {
      char tmp[100];
      snprintf(tmp, sizeof(tmp) - 1, "Unexpected response code: %d",
               source->GetResponseCode());
      error = tmp;
      break;
    }
  }

  VLOG(2) << "Device registration failed: " + error;

  RetryRegisterDeviceAfterDelay();
}

void SyncManagerImpl::RetryRegisterDeviceAfterDelay() {
  if (register_device_retry_)
    return;

  register_device_retry_ = new SyncRegisterRetry(this);
  ui_task_runner_->PostDelayedTask(
      FROM_HERE, base::Bind(&SyncRegisterRetry::Retry, register_device_retry_),
      base::TimeDelta::FromSeconds(kRegisterDeviceRetryDelayInSec));
}

void SyncManagerImpl::OnSyncStatusChanged(syncer::SyncService* sync_service) {
  const opera::SyncStatus status = sync_service->opera_sync_status();

  if (status.enabled()) {
    if (status.network_error()) {
      VLOG(2) << "Enabled with network error";
      UpdateStatus(STATUS_NETWORK_ERROR);
    } else {
      VLOG(2) << "Enabled";
      UpdateStatus(STATUS_RUNNING);
    }
  } else {
    if (status.disable_sync()) {
      VLOG(2) << "Disable sync";
      credentials_store_->Clear();
    }
    if (status.server_error()) {
      VLOG(2) << "Disabled because of server error";
      UpdateStatus(STATUS_FAILURE);
    } else if (!status.busy()) {
      VLOG(2) << "Disabled and not active";
      if (!credentials_store_->LoadLoginName().empty()) {
        UpdateStatus(STATUS_FAILURE);
      } else {
        UpdateStatus(STATUS_SETUP);
      }
    } else {
      VLOG(2) << "Disabled and but active";
    }
  }
}

// static
bool SyncManagerImpl::IsLoginErrorStatus(Status status) {
  switch (status) {
    case STATUS_RUNNING:
    case STATUS_NETWORK_ERROR:
    case STATUS_SETUP:
      break;
    case STATUS_FAILURE:
    case STATUS_LOGIN_ERROR:
      return true;
  }
  return false;
}

void SyncManagerImpl::UpdateStatus(Status status) {
  const Status old_status = status_;
  Status report;
  status_ = status;

  if (!opera::SyncConfig::UsesOAuth2()) {
    if (IsLoginErrorStatus(status_) && !IsLoginErrorStatus(old_status)) {
      VLOG(2) << "Got kicked out, try to login again";
      RetryLoginAfterDelay();
    }
  }

  if (IsLoginErrorStatus(status_) && login_request_ &&
      (login_retry_ || login_request_->Active())) {
    // Used as a "temporary error" while we retry
    VLOG(2) << "Login retry in progress";
    report = STATUS_NETWORK_ERROR;
  } else {
    report = status_;
  }

  if (reported_status_ != report) {
    reported_status_ = report;
    FOR_EACH_OBSERVER(SyncManager::Observer, observer_list_,
                      OnStatusChanged(report));
  }

  if (status_ == STATUS_RUNNING &&
      (old_status != STATUS_RUNNING || register_device_retry_)) {
    // If a retry is in action, try directly
    RegisterDevice();
  }
}

void SyncManagerImpl::AddObserver(SyncManager::Observer* observer) {
  observer_list_.AddObserver(observer);
}

void SyncManagerImpl::RemoveObserver(SyncManager::Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void SyncManagerImpl::Logout() {
  VLOG(2) << "Logout";
  credentials_store_->Clear();
  GetSyncService()->account()->LogOut(
      opera::account_util::LR_MOBILE_SYNC_MANAGER_LOGOUT);

  // Request a new login as the sync code will not do this
  auth_data_->RequestLogin("");

  // Make sure status change is noticed
  OnSyncStatusChanged(GetSyncService());

  Favorites::instance()->RemoveRemoteDevices();
  RemoveRemoteSessions();
}

std::string SyncManagerImpl::GetDisplayName() {
  return GetSyncService()->account()->login_data().display_name();
}

void SyncManagerImpl::ShowLoginPrompt(opera::SyncLoginUIParams params) {
  if (opera::SyncConfig::UsesOAuth2()) {
    NOTIMPLEMENTED();
    return;
  }

  if (GetSyncService()->account()->IsLoggedIn()) {
    VLOG(2) << "Show login prompt while logged in";
    return;
  }

  VLOG(2) << "Show login prompt";
  std::string name = GetSyncService()->account()->login_data().display_name();
  if (name.empty())
    name = credentials_store_->LoadLoginName();
  auth_data_->RequestLogin(name);
}

void SyncManagerImpl::ShowConnectionStatus(
    int status,
    Profile* profile,
    const opera::ShowSyncLoginPromptCallback& prompt_callback) {
  switch (status) {
    case opera::SYNC_CONNECTION_LOST:
      VLOG(2) << "Connection lost";
      break;
    case opera::SYNC_CONNECTION_RESTORED:
      VLOG(2) << "Connection restored";
      break;
    case opera::SYNC_CONNECTION_SERVER_ERROR:
      VLOG(2) << "Connection server error";
      if (status_ != STATUS_FAILURE) {
        UpdateStatus(STATUS_FAILURE);
      }
      prompt_callback.Run(opera::SYNC_LOGIN_UI_WITHOUT_SUPPRESS_OPTION);
      break;
    default:
      VLOG(2) << base::StringPrintf("Unknown connection status: %d", status);
  }
}

opera::AccountAuthDataFetcher* SyncManagerImpl::AuthDataFetcherFactory(
    const opera::AccountAuthData& old_data) {
  VLOG(2) << "Token renewal requested";
  opera::AccountAuthDataFetcher* fetcher =
      opera::CreateAuthDataUpdater(GetProfile()->GetRequestContext(), old_data);
  return fetcher;
}

void SyncManagerImpl::ReportLoginError() {
  UpdateStatus(STATUS_LOGIN_ERROR);
}

Profile* SyncManagerImpl::GetProfile() {
  return GetSyncProfile();
}

SyncProfile* SyncManagerImpl::GetSyncProfile() const {
  SyncProfileManager* profile_manager =
      static_cast<SyncProfileManager*>(g_browser_process->profile_manager());
  return profile_manager->profile();
}

void SyncManagerImpl::StartSessionRestore() {
  synced_window_delegate_->StartRestore();
}

void SyncManagerImpl::FinishSessionRestore() {
  synced_window_delegate_->FinishRestore();
}

void SyncManagerImpl::InsertTab(int index, const SyncedTabData* data) {
  synced_window_delegate_->InsertTab(index, data);
}

void SyncManagerImpl::RemoveTab(int index) {
  synced_window_delegate_->RemoveTab(index);
}

void SyncManagerImpl::UpdateTab(int index, const SyncedTabData* data) {
  synced_window_delegate_->UpdateTab(index, data);
}

void SyncManagerImpl::SetActiveTab(int index) {
  synced_window_delegate_->SetActiveTab(index);
}

void SyncManagerImpl::RetryLoginAfterDelay() {
  if (login_retry_)
    return;

  const base::TimeDelta delta = base::Time::Now() - last_login_retry_;

  // If the time since the last retry is more than 5 times longer than
  // the biggest delay, assume the last attempt worked and we now are
  // starting a new set of retries
  if (delta.InMilliseconds() >=
      kLoginRetryDelayInMilliSec[kMaxLoginRetries] * 5) {
    next_login_retry_index_ = 0;
  }

  size_t index = next_login_retry_index_;

  if (index >= kMaxLoginRetries) {
    VLOG(2) << "Too short time since last login retry, giving up";
    UpdateStatus(status_);
    return;
  }

  last_login_retry_ = base::Time::Now();
  ++next_login_retry_index_;
  login_retry_ = new SyncLoginRetry(this);
  ui_task_runner_->PostDelayedTask(
      FROM_HERE, base::Bind(&SyncLoginRetry::Retry, login_retry_),
      base::TimeDelta::FromMilliseconds(
          base::RandInt(kLoginRetryDelayInMilliSec[index],
                        kLoginRetryDelayInMilliSec[index + 1])));
}

bool SyncManagerImpl::TryToLogin(bool at_startup) {
  login_retry_ = NULL;

  if (!login_request_->Active() &&
      (IsLoginErrorStatus(status_) ||
       (at_startup && !GetSyncService()->account()->IsLoggedIn()))) {
    std::string login_name = credentials_store_->LoadLoginName();
    std::string user_name = credentials_store_->LoadUserName();
    if (!login_name.empty()) {
      std::string password = credentials_store_->LoadPassword(login_name);
      if (!password.empty()) {
        VLOG(2) << "Trying to login";
        login_request_->AuthDataLoggedIn(login_name, user_name, password);
        login_name.clear();
        user_name.clear();
        base::STLClearObject(&password);
        return true;
      }
    }
  }

  UpdateStatus(status_);  // Update reported status
  return false;
}

void SyncManagerImpl::InitializeFavorites(const std::string& session_name) {
  Favorites::instance()->SetDeviceName(session_name);
}

void SyncManagerImpl::Flush() {
  GetSyncService()->FlushDirectory();
  synced_window_delegate_->Flush();
}

void SyncManagerImpl::Shutdown() {
  GetSyncService()->Shutdown();
}

std::unique_ptr<syncer::DeviceInfo> SyncManagerImpl::GetDeviceInfoForName(
    const std::string& name) const {
  syncer::DeviceInfoTracker* tracker = GetSyncService()->GetDeviceInfoTracker();
  if (tracker) {
    std::vector<std::unique_ptr<syncer::DeviceInfo>> vector =
        tracker->GetAllDeviceInfo();
    for (auto& i : vector) {
      if (i->client_name() == name) {
        std::unique_ptr<syncer::DeviceInfo> device = std::move(i);
        return device;
      }
    }
  }
  return std::unique_ptr<syncer::DeviceInfo>();
}

void SyncManagerImpl::RemoveRemoteSessions() {
  auto service = GetSyncService();
  if (service) {
    auto delegate = service->GetOpenTabsUIDelegate();
    if (delegate) {
      std::vector<const sync_sessions::SyncedSession*> sessions;
      delegate->GetAllForeignSessions(&sessions);
      for (auto const& session : sessions) {
        delegate->DeleteForeignSession(session->session_tag);
      }
    }
  }
}

}  // namespace mobile
