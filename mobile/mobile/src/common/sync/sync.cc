// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sync.h"

#include <string>

#include "base/android/scoped_java_ref.h"
#include "base/android/jni_string.h"
#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "chrome/browser/invalidation/profile_invalidation_provider_factory.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/invalidation/impl/invalidation_service_android.h"
#include "components/invalidation/impl/profile_invalidation_provider.h"
#include "common/sync/invalidation.h"
#include "common/sync/sessions.h"
#include "mobile/common/bookmarks/bookmark_suggestion_provider.h"
#include "mobile/common/favorites/favorites.h"
#include "mobile/common/sync/auth_data.h"
#include "mobile/common/sync/sync_manager.h"
#include "mobile/common/sync/sync_manager_factory.h"
#include "mobile/common/sync/synced_tab_data.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

#include "jni/NativeSyncManager_jni.h"

namespace mobile {

namespace {

class SyncManagerRequest {
 public:
  SyncManagerRequest();
  ~SyncManagerRequest();

  virtual bool IsCurrent(jint id) const;
  virtual bool Cancel();

 protected:
  bool SetCurrent(jint id);

 private:
  jint current_;
};

SyncManagerRequest::SyncManagerRequest()
    : current_(0) {
}

SyncManagerRequest::~SyncManagerRequest() {
  Cancel();
}

bool SyncManagerRequest::Cancel() {
  if (current_) {
    jint id = current_;
    current_ = 0;
    android::Java_NativeSyncManager_cancelRequest(
        base::android::AttachCurrentThread(), id);
    return true;
  }
  return false;
}

bool SyncManagerRequest::IsCurrent(jint id) const {
  DCHECK(id);
  return current_ == id;
}

bool SyncManagerRequest::SetCurrent(jint id) {
  if (id) {
    Cancel();
    current_ = id;
    return true;
  } else if (current_) {
    current_ = 0;
    return true;
  }
  return false;
}

class SyncAuthData : public AuthData, public SyncManagerRequest {
 public:
  SyncAuthData();
  ~SyncAuthData();
  void Setup(const std::string& application,
             const std::string& application_key,
             const std::string& client_key,
             const std::string& client_secret) override;
  void RequestLogin(const std::string& login_name) override;
  void RequestToken(const std::string& login_name,
                    const std::string& password) override;

  void LoggedIn(const std::string& login_name,
                const std::string& user_name,
                const std::string& password) {
    AuthData::LoggedIn(login_name, user_name, password);
  }

  void GotToken(const std::string& token, const std::string& secret) {
    AuthData::GotToken(token, secret);
  }

  void Login(const std::string& token,
             const std::string& secret,
             const std::string& display_name) {
    AuthData::Login(token, secret, display_name);
  }

  void GotError(Error error, const std::string& message) {
    AuthData::GotError(error, message);
  }
};

SyncAuthData* g_sync_auth;

SyncAuthData::SyncAuthData() {
  DCHECK(g_sync_auth == nullptr);
  g_sync_auth = this;
}

SyncAuthData::~SyncAuthData() {
  DCHECK(g_sync_auth == this);
  g_sync_auth = nullptr;
}

void SyncAuthData::Setup(const std::string& application,
                         const std::string& application_key,
                         const std::string& client_key,
                         const std::string& client_secret) {
  JNIEnv* env = base::android::AttachCurrentThread();
  android::Java_NativeSyncManager_setupOAuth(
      env,
      base::android::ConvertUTF8ToJavaString(env, application).obj(),
      base::android::ConvertUTF8ToJavaString(env, application_key).obj(),
      base::android::ConvertUTF8ToJavaString(env, client_key).obj(),
      base::android::ConvertUTF8ToJavaString(env, client_secret).obj());
}

void SyncAuthData::RequestLogin(const std::string& login_name) {
  JNIEnv* env = base::android::AttachCurrentThread();
  android::Java_NativeSyncManager_requestLogin(
      env, base::android::ConvertUTF8ToJavaString(env, login_name).obj());
}

void SyncAuthData::RequestToken(const std::string& login_name,
                                const std::string& password) {
  JNIEnv* env = base::android::AttachCurrentThread();
  SetCurrent(android::Java_NativeSyncManager_requestToken(
      env,
      base::android::ConvertUTF8ToJavaString(env, login_name).obj(),
      base::android::ConvertUTF8ToJavaString(env, password).obj()));
}

AuthData::Error ConvertBreamErrorToAuthDataError(jint error) {
  switch (error) {
    case 0:  // PASSWORD_INVALID
      return AuthData::PASSWORD_INVALID;
    case 1:  // EMAIL_MISSING
      return AuthData::EMAIL_MISSING;
    case 2:  // EMAIL_INVALID
      return AuthData::EMAIL_INVALID;
    case 3:  // EMAIL_INUSE
      return AuthData::EMAIL_INUSE;
    case 4:  // BAD_CREDENTIALS
      return AuthData::BAD_CREDENTIALS;
    case 5:  // NETWORK_ERROR
      break;
    case 6:  // TIMEDOUT
      return AuthData::TIMEDOUT;
  }
  return AuthData::NETWORK_ERROR;
}

jint ConvertSyncManagerStatusToJava(SyncManager::Status status) {
  switch (status) {
    case SyncManager::STATUS_SETUP:
      return 0;
    case SyncManager::STATUS_RUNNING:
      return 1;
    case SyncManager::STATUS_FAILURE:
      return 2;
    case SyncManager::STATUS_NETWORK_ERROR:
      return 3;
    case SyncManager::STATUS_LOGIN_ERROR:
      return 4;
  }
  return 0;
}

Sync* g_sync;

jclass g_SyncedTabData_clazz;
jmethodID g_SyncedTabData_getCPtr;

#define kSyncedTabDataClassPath "com/opera/android/op/SyncedTabData"

}  // namespace

// static
Sync* Sync::GetInstance() {
  if (!g_sync) {
    g_sync = new Sync();
  }
  return g_sync;
}

// static
void Sync::Shutdown() {
  delete g_sync;
  g_sync = nullptr;
}

Sync::Sync() {
}

Sync::~Sync() {
  if (sync_manager_)
    sync_manager_->RemoveObserver(this);
}

void Sync::AddObserver(SyncObserver* observer) {
  observers_.AddObserver(observer);
}

void Sync::RemoveObserver(SyncObserver* observer) {
  observers_.RemoveObserver(observer);
}

void Sync::AuthSetup() {
  sync_manager_->AuthSetup();
}

void Sync::Initialize(
    base::FilePath base_path,
    scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    content::BrowserContext* browser_context) {
  DCHECK(!sync_manager_);
  invalidation_.reset(new Invalidation());
  auth_data_.reset(new SyncAuthData());

  sync_manager_.reset(SyncManagerFactory::Create(base_path,
                                                 ui_task_runner,
                                                 io_task_runner,
                                                 browser_context,
                                                 invalidation_.get(),
                                                 auth_data_.get()));
  sync_manager_->AddObserver(this);
  FOR_EACH_OBSERVER(SyncObserver, observers_, SyncIsReady());
}

bookmarks::BookmarkModel* Sync::GetBookmarkModel() {
  DCHECK(sync_manager_);
  return sync_manager_->GetBookmarkModel();
}

opera::SuggestionProvider* Sync::GetBookmarkSuggestionProvider() {
  DCHECK(sync_manager_);
  if (!bookmark_suggestion_provider_) {
    bookmark_suggestion_provider_.reset(
        new BookmarkSuggestionProvider(sync_manager_->GetBookmarkModel()));
  }
  return bookmark_suggestion_provider_.get();
}

SyncManager* Sync::GetSyncManager() {
  return sync_manager_.get();
}

Invalidation* Sync::GetInvalidation() {
  return invalidation_.get();
}

void Sync::OnStatusChanged(SyncManager::Status newStatus) {
  JNIEnv* env = base::android::AttachCurrentThread();
  android::Java_NativeSyncManager_statusChanged(
      env, ConvertSyncManagerStatusToJava(newStatus));
}


Profile* Sync::GetProfile() {
  return sync_manager_->GetProfile();
}

// static
bool Sync::RegisterJni(JNIEnv* env) {
  g_SyncedTabData_clazz = reinterpret_cast<jclass>(env->NewGlobalRef(
      base::android::GetClass(env, kSyncedTabDataClassPath).obj()));
  g_SyncedTabData_getCPtr =
      base::android::MethodID::Get<base::android::MethodID::TYPE_STATIC>(
          env,
          g_SyncedTabData_clazz,
          "getCPtr",
          "(L" kSyncedTabDataClassPath ";)J");

  return android::RegisterNativesImpl(env) && Sessions::RegisterJni(env);
}

namespace android {

// static
void SetDeviceId(JNIEnv* env,
                 const base::android::JavaParamRef<jclass>& clazz,
                 const base::android::JavaParamRef<jstring>& id) {
  std::string device_id = base::android::ConvertJavaStringToUTF8(env, id);
  Sync::GetInstance()->GetInvalidation()->SetDeviceId(device_id);
}

// static
jint GetStatus(JNIEnv* env, const base::android::JavaParamRef<jclass>& clazz) {
  return Sync::GetInstance()->GetSyncManager()->GetStatus();
}

// static
base::android::ScopedJavaLocalRef<jstring> GetDisplayName(
    JNIEnv* env,
    const base::android::JavaParamRef<jclass>& clazz) {
  std::string display_name =
      Sync::GetInstance()->GetSyncManager()->GetDisplayName();
  return base::android::ConvertUTF8ToJavaString(env, display_name);
}

// static
void InvalidateAll(JNIEnv* env,
                   const base::android::JavaParamRef<jclass>& clazz) {
  Sync::GetInstance()->GetSyncManager()->InvalidateAll();
}

// static
void Invalidate(JNIEnv* env,
                const base::android::JavaParamRef<jclass>& clazz,
                jint source,
                const base::android::JavaParamRef<jstring>& id,
                jlong version,
                const base::android::JavaParamRef<jstring>& payload) {
  static_cast<invalidation::InvalidationServiceAndroid*>(
      invalidation::ProfileInvalidationProviderFactory::GetForProfile(
          Sync::GetInstance()->GetSyncManager()->GetProfile())->
      GetInvalidationService())->
      Invalidate(env, nullptr, source, id, version, payload);
}

// static
void Logout(JNIEnv* env, const base::android::JavaParamRef<jclass>& clazz) {
  SyncManager* sync_manager = Sync::GetInstance()->GetSyncManager();
  sync_manager->Logout();
  Favorites::instance()->ProtectRootFromSyncMerge();
  sync_manager->GetBookmarkModel()->CommitPendingChanges();
}

// static
void LoggedIn(JNIEnv* env,
              const base::android::JavaParamRef<jclass>& clazz,
              const base::android::JavaParamRef<jstring>& login_name,
              const base::android::JavaParamRef<jstring>& user_name,
              const base::android::JavaParamRef<jstring>& password) {
  if (!g_sync_auth)
    return;
  g_sync_auth->LoggedIn(base::android::ConvertJavaStringToUTF8(env, login_name),
                        base::android::ConvertJavaStringToUTF8(env, user_name),
                        base::android::ConvertJavaStringToUTF8(env, password));
}

// static
void GotToken(JNIEnv* env,
              const base::android::JavaParamRef<jclass>& clazz,
              const base::android::JavaParamRef<jstring>& token,
              const base::android::JavaParamRef<jstring>& secret) {
  if (!g_sync_auth)
    return;
  g_sync_auth->GotToken(base::android::ConvertJavaStringToUTF8(env, token),
                        base::android::ConvertJavaStringToUTF8(env, secret));
}

// static
void Login(JNIEnv* env,
           const base::android::JavaParamRef<jclass>& caller,
           const base::android::JavaParamRef<jstring>& token,
           const base::android::JavaParamRef<jstring>& secret,
           const base::android::JavaParamRef<jstring>& display_name) {
  if (!g_sync_auth)
    return;
  g_sync_auth->Login(base::android::ConvertJavaStringToUTF8(env, token),
                     base::android::ConvertJavaStringToUTF8(env, secret),
                     base::android::ConvertJavaStringToUTF8(env, display_name));
}

// static
void Error(JNIEnv* env,
           const base::android::JavaParamRef<jclass>& clazz,
           jint id,
           jint error,
           const base::android::JavaParamRef<jstring>& message) {
  if (g_sync_auth && g_sync_auth->IsCurrent(id)) {
    g_sync_auth->GotError(ConvertBreamErrorToAuthDataError(error),
                          base::android::ConvertJavaStringToUTF8(env, message));
  }
}

// static
base::android::ScopedJavaLocalRef<jobjectArray> GetSyncedSessions(
    JNIEnv* env,
    const base::android::JavaParamRef<jclass>& clazz) {
  return Sessions::GetSessions(env);
}

// static
void StartSessionRestore(JNIEnv* env,
                         const base::android::JavaParamRef<jclass>& clazz) {
  Sync::GetInstance()->GetSyncManager()->StartSessionRestore();
}

// static
void FinishSessionRestore(JNIEnv* env,
                          const base::android::JavaParamRef<jclass>& clazz) {
  Sync::GetInstance()->GetSyncManager()->FinishSessionRestore();
}

// static
void InsertTab(JNIEnv* env,
               const base::android::JavaParamRef<jclass>& clazz,
               int index,
               const base::android::JavaParamRef<jobject>& data) {
  const SyncedTabData* ptr =
      reinterpret_cast<SyncedTabData*>(
          env->CallStaticLongMethod(g_SyncedTabData_clazz,
                                    g_SyncedTabData_getCPtr,
                                    data.obj()));
  Sync::GetInstance()->GetSyncManager()->InsertTab(index, ptr);
}

// static
void RemoveTab(JNIEnv* env,
               const base::android::JavaParamRef<jclass>& clazz,
               int index) {
  Sync::GetInstance()->GetSyncManager()->RemoveTab(index);
}

// static
void UpdateTab(JNIEnv* env,
               const base::android::JavaParamRef<jclass>& clazz,
               int index,
               const base::android::JavaParamRef<jobject>& data) {
  const SyncedTabData* ptr =
      reinterpret_cast<SyncedTabData*>(env->CallStaticLongMethod(
          g_SyncedTabData_clazz, g_SyncedTabData_getCPtr, data.obj()));
  Sync::GetInstance()->GetSyncManager()->UpdateTab(index, ptr);
}

// static
void SetActiveTab(JNIEnv* env,
                  const base::android::JavaParamRef<jclass>& clazz,
                  int index) {
  Sync::GetInstance()->GetSyncManager()->SetActiveTab(index);
}

}  // namespace android

}  // namespace mobile
