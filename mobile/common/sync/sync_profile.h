// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef MOBILE_COMMON_SYNC_SYNC_PROFILE_H_
#define MOBILE_COMMON_SYNC_SYNC_PROFILE_H_

#include <memory>
#include <utility>

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "chrome/browser/profiles/profile.h"
#include "components/syncable_prefs/pref_service_syncable.h"

class PrefService;

namespace net {
class URLRequestContextGetter;
}

namespace mobile {

class SyncProfile : public Profile {
 public:
  SyncProfile(content::BrowserContext* context,
              scoped_refptr<base::SequencedTaskRunner> io_task_runner);

  base::FilePath GetPath() const override { return context_->GetPath(); }

  bool IsOffTheRecord() const override { return context_->IsOffTheRecord(); }

  net::URLRequestContextGetter* GetRequestContext() override;

  bool IsSupervised() override { return false; }

  content::ResourceContext* GetResourceContext() override {
    return context_->GetResourceContext();
  }

  content::DownloadManagerDelegate* GetDownloadManagerDelegate() override {
    return context_->GetDownloadManagerDelegate();
  }

  content::BrowserPluginGuestManager* GetGuestManager() override {
    return context_->GetGuestManager();
  }

  storage::SpecialStoragePolicy* GetSpecialStoragePolicy() override {
    return context_->GetSpecialStoragePolicy();
  }

  content::PushMessagingService* GetPushMessagingService() override {
    return context_->GetPushMessagingService();
  }

  content::SSLHostStateDelegate* GetSSLHostStateDelegate() override {
    return context_->GetSSLHostStateDelegate();
  }

  content::PermissionManager* GetPermissionManager() override {
    return context_->GetPermissionManager();
  }

  std::unique_ptr<content::ZoomLevelDelegate> CreateZoomLevelDelegate(
      const base::FilePath& partition_path) override {
    return context_->CreateZoomLevelDelegate(partition_path);
  }

  content::BackgroundSyncController* GetBackgroundSyncController() override {
    return context_->GetBackgroundSyncController();
  }

  net::URLRequestContextGetter* CreateRequestContext(
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors)
      override {
    return context_->CreateRequestContext(protocol_handlers,
                                          std::move(request_interceptors));
  }

  net::URLRequestContextGetter* CreateRequestContextForStoragePartition(
      const base::FilePath& partition_path,
      bool in_memory,
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors)
      override {
    return context_->CreateRequestContextForStoragePartition(
        partition_path, in_memory, protocol_handlers,
        std::move(request_interceptors));
  }

  net::URLRequestContextGetter* CreateMediaRequestContext() override {
    return context_->CreateMediaRequestContext();
  }

  net::URLRequestContextGetter* CreateMediaRequestContextForStoragePartition(
      const base::FilePath& partition_path,
      bool in_memory) override {
    return context_->CreateMediaRequestContextForStoragePartition(
        partition_path, in_memory);
  }

  scoped_refptr<base::SequencedTaskRunner> GetIOTaskRunner() override {
    return io_task_runner_;
  }

  PrefService* GetPrefs() override { return pref_service_.get(); }

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  content::BrowserContext* GetBrowserContext() const { return context_; }

 private:
  content::BrowserContext* context_;
  scoped_refptr<user_prefs::PrefRegistrySyncable> pref_registry_;
  std::unique_ptr<PrefService> pref_service_;
  scoped_refptr<base::SequencedTaskRunner> io_task_runner_;
};

}  // namespace mobile

#endif  // MOBILE_COMMON_SYNC_SYNC_PROFILE_H_
