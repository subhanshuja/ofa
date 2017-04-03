// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/sync_stopped_reporter.h"

#if defined(OPERA_SYNC)
#include <string>
#endif  // OPERA_SYNC

#include "base/location.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/sync/protocol/sync.pb.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"

#if defined(OPERA_SYNC)
#include "base/message_loop/message_loop.h"
#include "chrome/browser/profiles/profile.h"
#include "components/sync/device_info/local_device_info_provider_impl.h"

#include "common/account/account_service.h"
#include "common/sync/sync_config.h"
#endif  // OPERA_SYNC

namespace {

const char kEventEndpoint[] = "event";

// The request is tiny, so even on poor connections 10 seconds should be
// plenty of time. Since sync is off when this request is started, we don't
// want anything sync-related hanging around for very long from a human
// perspective either. This seems like a good compromise.
const int kRequestTimeoutSeconds = 10;

}  // namespace

namespace syncer {

SyncStoppedReporter::SyncStoppedReporter(
    const GURL& sync_service_url,
    const std::string& user_agent,
    const scoped_refptr<net::URLRequestContextGetter>& request_context,
    const ResultCallback& callback)
    : sync_event_url_(GetSyncEventURL(sync_service_url)),
      user_agent_(user_agent),
      request_context_(request_context),
      callback_(callback) {
  DCHECK(!sync_service_url.is_empty());
  DCHECK(!user_agent_.empty());
  DCHECK(request_context);
}

SyncStoppedReporter::~SyncStoppedReporter() {}

#if defined(OPERA_SYNC)
void SyncStoppedReporter::SetSyncAccount(const opera::SyncAccount* account) {
  DCHECK(account);
  account_ = account;
}

void SyncStoppedReporter::DoCacheAuthHeaderNow() {
  DCHECK(!opera::SyncConfig::UsesOAuth2());
  DCHECK(account_);
  DCHECK(account_->service());

  if (account_->IsLoggedIn() && account_->service()->HasAuthToken()) {
    cached_auth_header_ = account_->service()->GetSignedAuthHeader(
       sync_event_url_, opera::AccountService::HTTP_METHOD_POST,
       opera::SyncConfig::AuthServerURL().host());
  } else {
    VLOG(2) << "Cannot cache auth header since account is logged out!";
  }
}
#endif  // OPERA_SYNC

void SyncStoppedReporter::ReportSyncStopped(const std::string& access_token,
                                            const std::string& cache_guid,
                                            const std::string& birthday) {
  DCHECK(!access_token.empty());
  DCHECK(!cache_guid.empty());
  DCHECK(!birthday.empty());

  // Make the request proto with the GUID identifying this client.
  sync_pb::EventRequest event_request;
  sync_pb::SyncDisabledEvent* sync_disabled_event =
      event_request.mutable_sync_disabled();
  sync_disabled_event->set_cache_guid(cache_guid);
  sync_disabled_event->set_store_birthday(birthday);

  std::string msg;
  event_request.SerializeToString(&msg);

  fetcher_ =
      net::URLFetcher::Create(sync_event_url_, net::URLFetcher::POST, this);

#if defined(OPERA_SYNC)
  if (!opera::SyncConfig::UsesOAuth2()) {
    // Just in case...
    DoCacheAuthHeaderNow();

    if (cached_auth_header_.empty()) {
      LOG(WARNING) << "Cannot sign the sync stopped report, not seding one.";
      return;
    }
    VLOG(3) << "Sync stopped event signed with " << cached_auth_header_;
  }

  std::string authorization_header;
  if (opera::SyncConfig::UsesOAuth2()) {
    authorization_header = base::StringPrintf(
        "%s: Bearer %s", net::HttpRequestHeaders::kAuthorization,
        access_token.c_str());
  } else {
    authorization_header =
        base::StringPrintf("%s: %s", net::HttpRequestHeaders::kAuthorization,
                           cached_auth_header_.c_str());
  }
  fetcher_->AddExtraRequestHeader(authorization_header);
#else
  fetcher_->AddExtraRequestHeader(base::StringPrintf(
      "%s: Bearer %s", net::HttpRequestHeaders::kAuthorization,
      access_token.c_str()));
#endif  // OPERA_SYNC
  fetcher_->AddExtraRequestHeader(base::StringPrintf(
      "%s: %s", net::HttpRequestHeaders::kUserAgent, user_agent_.c_str()));
  fetcher_->SetRequestContext(request_context_.get());
  fetcher_->SetUploadData("application/octet-stream", msg);
  fetcher_->SetLoadFlags(net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
                         net::LOAD_DO_NOT_SAVE_COOKIES |
                         net::LOAD_DO_NOT_SEND_COOKIES);
  fetcher_->Start();
  timer_.Start(FROM_HERE, base::TimeDelta::FromSeconds(kRequestTimeoutSeconds),
               this, &SyncStoppedReporter::OnTimeout);
}

void SyncStoppedReporter::OnURLFetchComplete(const net::URLFetcher* source) {
  Result result =
      source->GetResponseCode() == net::HTTP_OK ? RESULT_SUCCESS : RESULT_ERROR;
  fetcher_.reset();
  timer_.Stop();
  if (!callback_.is_null()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(callback_, result));
  }
}

void SyncStoppedReporter::OnTimeout() {
  fetcher_.reset();
  if (!callback_.is_null()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(callback_, RESULT_TIMEOUT));
  }
}

// Static.
GURL SyncStoppedReporter::GetSyncEventURL(const GURL& sync_service_url) {
  std::string path = sync_service_url.path();
  if (path.empty() || *path.rbegin() != '/') {
    path += '/';
  }
  path += kEventEndpoint;
  GURL::Replacements replacements;
  replacements.SetPathStr(path);
  return sync_service_url.ReplaceComponents(replacements);
}

void SyncStoppedReporter::SetTimerTaskRunnerForTest(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner) {
  timer_.SetTaskRunner(task_runner);
}

}  // namespace syncer
