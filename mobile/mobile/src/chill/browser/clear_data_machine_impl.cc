// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// NOLINTNEXTLINE(whitespace/line_length)
// @copied-from chromium/src/chrome/browser/browsing_data/browsing_data_remover.cc
// @final-synchronized

#include "chill/browser/clear_data_machine.h"

#include <set>
#include <vector>

#include "base/memory/ref_counted.h"

#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/dom_storage_context.h"
#include "content/public/browser/local_storage_usage_info.h"
#include "content/public/browser/session_storage_usage_info.h"
#include "content/public/browser/storage_partition.h"

#include "net/cookies/cookie_store.h"
#include "net/disk_cache/disk_cache.h"
#include "net/http/http_cache.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

#include "storage/browser/quota/quota_manager.h"
#include "storage/common/quota/quota_types.h"

using content::BrowserThread;

namespace opera {

class ClearDataMachineImpl :
    public ClearDataMachine {
 public:
  virtual void ScheduleClearData(Instruction instruction);

  virtual void Clear();

  ClearDataMachineImpl(content::BrowserContext* browser_context,
                       net::URLRequestContextGetter* context_getter);

 private:
  bool ClearAllDone() const;

  void ClearCache();
  void ClearCookies();
  void ClearWebStorage();
  void ClearAuthCache();

  void OnAllCleared();
  void OnCacheCleared();
  void OnCookiesCleared(int num_deleted);
  void OnLocalStorageCleared();
  void OnQuotaManagedDataCleared();
  void OnSessionStorageCleared();
  void OnAuthCacheCleared();

  // Clear cache states
  void ClearCacheClear(int rv);
  void ClearCacheDone(int rv);

  disk_cache::Backend* cache_;

  bool ClearQuotaDone() const;

  void ClearLocalStorageClear(
      const std::vector<content::LocalStorageUsageInfo>& infos);
  void ClearQuotaManagedDataClear();
  void DidClearHostsQuotas(bool success);
  void ClearQuotaManagedOriginsClear(
      const std::set<GURL>& origins, storage::StorageType type);
  void ClearQuotaManagedOriginsDelete(
      const GURL& origin,
      storage::StorageType type,
      storage::QuotaStatusCode status);
  void ClearSessionStorageClear(
      const std::vector<content::SessionStorageUsageInfo>& infos);

  content::DOMStorageContext* dom_storage_context_;
  storage::QuotaManager* quota_manager_;
  bool cleared_hosts_quotas_;
  int quota_managed_origins_;
  int quota_managed_storage_types_;

  content::BrowserContext* browser_context_;
  scoped_refptr<net::URLRequestContextGetter> context_getter_;

  bool clearing_cache_done_;
  bool clearing_cookies_done_;
  bool clearing_local_storage_done_;
  bool clearing_quota_managed_data_done_;
  bool clearing_session_storage_done_;
  bool clearing_auth_cache_done_;
};

ClearDataMachine* ClearDataMachine::CreateClearDataMachine(
    content::BrowserContext* browser_context) {
  return new ClearDataMachineImpl(
      browser_context,
      content::BrowserContext::GetDefaultStoragePartition(
          browser_context)->GetURLRequestContext());
}

void ClearDataMachineImpl::Clear() {
  if (ClearAllDone()) {
    OnAllCleared();
    return;
  }

  if (!clearing_cache_done_) {
    ClearCache();
  }

  if (!clearing_cookies_done_) {
    ClearCookies();
  }

  if (!clearing_local_storage_done_ ||
      !clearing_quota_managed_data_done_ ||
      !clearing_session_storage_done_) {
    ClearWebStorage();
  }

  if (!clearing_auth_cache_done_) {
    ClearAuthCache();
  }
}

void ClearDataMachineImpl::ScheduleClearData(Instruction instruction) {
  switch (instruction) {
    case CLEAR_CACHE: {
      clearing_cache_done_ = false;
      break;
    }
    case CLEAR_COOKIES: {
      clearing_cookies_done_ = false;
      break;
    }
    case CLEAR_WEB_STORAGE: {
      clearing_local_storage_done_ = false;
      clearing_quota_managed_data_done_ = false;
      clearing_session_storage_done_ = false;
      break;
    }
    case CLEAR_AUTH_CACHE: {
      clearing_auth_cache_done_ = false;
      break;
    }
    case CLEAR_ALL: {
      clearing_cache_done_ = false;
      clearing_cookies_done_ = false;
      clearing_local_storage_done_ = false;
      clearing_quota_managed_data_done_ = false;
      clearing_session_storage_done_ = false;
      clearing_auth_cache_done_ = false;
      break;
    }
  }
}

ClearDataMachineImpl::ClearDataMachineImpl(
    content::BrowserContext* browser_context,
    net::URLRequestContextGetter* context_getter)
    : cache_(NULL),
      dom_storage_context_(NULL),
      quota_manager_(NULL),
      cleared_hosts_quotas_(true),
      quota_managed_origins_(0),
      quota_managed_storage_types_(0),
      browser_context_(browser_context),
      context_getter_(context_getter),
      clearing_cache_done_(true),
      clearing_cookies_done_(true),
      clearing_local_storage_done_(true),
      clearing_quota_managed_data_done_(true),
      clearing_session_storage_done_(true),
      clearing_auth_cache_done_(true) {
}

bool ClearDataMachineImpl::ClearAllDone() const {
  return clearing_cache_done_ &&
      clearing_cookies_done_ &&
      clearing_local_storage_done_ &&
      clearing_quota_managed_data_done_ &&
      clearing_session_storage_done_ &&
      clearing_auth_cache_done_;
}

void ClearDataMachineImpl::ClearCache() {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&ClearDataMachineImpl::ClearCache,
                   base::Unretained(this)));
    return;
  }

  net::HttpTransactionFactory* factory =
      context_getter_->GetURLRequestContext()->http_transaction_factory();

  bool async = factory->GetCache()->GetBackend(
      &cache_, base::Bind(&ClearDataMachineImpl::ClearCacheClear,
                          base::Unretained(this))) == net::ERR_IO_PENDING;

  if (!async) {
    ClearCacheClear(net::OK);
  }
}

void ClearDataMachineImpl::ClearCookies() {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&ClearDataMachineImpl::ClearCookies,
                   base::Unretained(this)));
    return;
  }

  if (context_getter_) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
    net::CookieStore* cookie_store = context_getter_->
        GetURLRequestContext()->cookie_store();
    cookie_store->DeleteAllCreatedBetweenAsync(
        base::Time(), base::Time::Max(),
        base::Bind(&ClearDataMachineImpl::OnCookiesCleared,
                   base::Unretained(this)));
  }
}

void ClearDataMachineImpl::ClearWebStorage() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  content::StoragePartition* partition =
      content::BrowserContext::GetStoragePartition(browser_context_, NULL);

  if (!dom_storage_context_) {
    dom_storage_context_ = partition->GetDOMStorageContext();
  }

  dom_storage_context_->GetLocalStorageUsage(
      base::Bind(&ClearDataMachineImpl::ClearLocalStorageClear,
                 base::Unretained(this)));

  dom_storage_context_->GetSessionStorageUsage(
      base::Bind(&ClearDataMachineImpl::ClearSessionStorageClear,
                 base::Unretained(this)));

  if (!quota_manager_) {
    quota_manager_ = partition->GetQuotaManager();
  }

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&ClearDataMachineImpl::ClearQuotaManagedDataClear,
                 base::Unretained(this)));
}

void ClearDataMachineImpl::ClearAuthCache() {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&ClearDataMachineImpl::ClearAuthCache,
                   base::Unretained(this)));
    return;
  }

  net::HttpTransactionFactory* http_transaction_factory =
      context_getter_->GetURLRequestContext()->http_transaction_factory();
  if (!http_transaction_factory)
    return;

  net::HttpNetworkSession* http_network_session =
      http_transaction_factory->GetSession();
  if (!http_network_session)
    return;

  net::HttpAuthCache* http_auth_cache =
    http_network_session->http_auth_cache();
  http_auth_cache->ClearEntriesAddedWithin(base::Time::Now() - base::Time());

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&ClearDataMachineImpl::OnAuthCacheCleared,
                 base::Unretained(this)));
}

void ClearDataMachineImpl::OnAllCleared() {
  BrowserThread::DeleteSoon(BrowserThread::UI, FROM_HERE, this);
}

void ClearDataMachineImpl::OnCacheCleared() {
  clearing_cache_done_ = true;
  if (ClearAllDone()) {
    OnAllCleared();
  }
}

void ClearDataMachineImpl::OnCookiesCleared(int num_deleted) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&ClearDataMachineImpl::OnCookiesCleared,
                   base::Unretained(this), num_deleted));
    return;
  }

  clearing_cookies_done_ = true;
  if (ClearAllDone()) {
    OnAllCleared();
  }
}

void ClearDataMachineImpl::OnLocalStorageCleared() {
  clearing_local_storage_done_ = true;
  if (ClearAllDone()) {
    OnAllCleared();
  }
}

void ClearDataMachineImpl::OnQuotaManagedDataCleared() {
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&ClearDataMachineImpl::OnQuotaManagedDataCleared,
                   base::Unretained(this)));
    return;
  }

  clearing_quota_managed_data_done_ = true;
  if (ClearAllDone()) {
    OnAllCleared();
  }
}

void ClearDataMachineImpl::OnSessionStorageCleared() {
  clearing_session_storage_done_ = true;
  if (ClearAllDone()) {
    OnAllCleared();
  }
}

void ClearDataMachineImpl::OnAuthCacheCleared() {
  clearing_auth_cache_done_ = true;
  if (ClearAllDone()) {
    OnAllCleared();
  }
}

// Clear cache states
void ClearDataMachineImpl::ClearCacheClear(int rv) {
  bool async = false;
  if (cache_) {
    async = cache_->DoomAllEntries(
        base::Bind(&ClearDataMachineImpl::ClearCacheDone,
                   base::Unretained(this))) == net::ERR_IO_PENDING;
  }

  if (!async) {
    ClearCacheDone(net::OK);
  }
}

void ClearDataMachineImpl::ClearCacheDone(int rv) {
  cache_ = NULL;

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&ClearDataMachineImpl::OnCacheCleared,
                 base::Unretained(this)));
}

bool ClearDataMachineImpl::ClearQuotaDone() const {
  return quota_managed_storage_types_ == 0 && quota_managed_origins_ == 0
      && cleared_hosts_quotas_;
}

void ClearDataMachineImpl::ClearLocalStorageClear(
    const std::vector<content::LocalStorageUsageInfo>& infos) {
  for (size_t i = 0; i < infos.size(); ++i) {
    dom_storage_context_->DeleteLocalStorage(infos[i].origin);
  }

  OnLocalStorageCleared();
}

void ClearDataMachineImpl::ClearQuotaManagedDataClear() {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&ClearDataMachineImpl::ClearQuotaManagedDataClear,
                   base::Unretained(this)));
    return;
  }

  cleared_hosts_quotas_ = false;
  quota_managed_origins_ = 0;
  quota_managed_storage_types_ = 2;

  quota_manager_->GetOriginsModifiedSince(
      storage::kStorageTypePersistent, base::Time(),
      base::Bind(&ClearDataMachineImpl::ClearQuotaManagedOriginsClear,
                 base::Unretained(this)));

  quota_manager_->GetOriginsModifiedSince(
      storage::kStorageTypeTemporary, base::Time(),
      base::Bind(&ClearDataMachineImpl::ClearQuotaManagedOriginsClear,
                 base::Unretained(this)));

  quota_manager_->ClearHostsQuotas(
      base::Bind(&ClearDataMachineImpl::DidClearHostsQuotas,
                 base::Unretained(this)));
}

void ClearDataMachineImpl::DidClearHostsQuotas(bool success) {
  if (!success) {
    DLOG(ERROR) << "Failed to clear hosts quotas";
  }

  cleared_hosts_quotas_ = true;
  if (ClearQuotaDone()) {
    OnQuotaManagedDataCleared();
  }
}

void ClearDataMachineImpl::ClearQuotaManagedOriginsClear(
    const std::set<GURL>& origins, storage::StorageType type) {

  std::set<GURL>::const_iterator origin;
  for (origin = origins.begin(); origin != origins.end(); ++origin) {
    ++quota_managed_origins_;
    quota_manager_->DeleteOriginData(
        origin->GetOrigin(), type,
        storage::QuotaClient::kAllClientsMask,
        base::Bind(&ClearDataMachineImpl::ClearQuotaManagedOriginsDelete,
                   base::Unretained(this), origin->GetOrigin(), type));
  }

  --quota_managed_storage_types_;
  if (ClearQuotaDone()) {
    OnQuotaManagedDataCleared();
  }
}

void ClearDataMachineImpl::ClearQuotaManagedOriginsDelete(
    const GURL& origin,
    storage::StorageType type,
    storage::QuotaStatusCode status) {
  if (status != storage::kQuotaStatusOk) {
    DLOG(ERROR) << "Couldn't remove data of type " << type << " for origin "
                << origin << ". Status: " << status;
  }

  --quota_managed_origins_;
  if (ClearQuotaDone()) {
    OnQuotaManagedDataCleared();
  }
}

void ClearDataMachineImpl::ClearSessionStorageClear(
    const std::vector<content::SessionStorageUsageInfo>& infos) {
  for (size_t i = 0; i < infos.size(); ++i) {
    dom_storage_context_->DeleteSessionStorage(infos[i]);
  }

  OnSessionStorageCleared();
}

}  // namespace opera
