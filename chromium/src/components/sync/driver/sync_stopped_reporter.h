// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_DRIVER_SYNC_STOPPED_REPORTER_H_
#define COMPONENTS_SYNC_DRIVER_SYNC_STOPPED_REPORTER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/timer/timer.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

#if defined(OPERA_SYNC)
#include "common/sync/sync_account.h"
#endif  // OPERA_SYNC

namespace syncer {

// Manages informing the sync server that sync has been disabled.
// An implementation of URLFetcherDelegate was needed in order to
// clean up the fetcher_ pointer when the request completes.
class SyncStoppedReporter : public net::URLFetcherDelegate {
 public:
  enum Result { RESULT_SUCCESS, RESULT_ERROR, RESULT_TIMEOUT };

  typedef base::Callback<void(const Result&)> ResultCallback;

  SyncStoppedReporter(
      const GURL& sync_service_url,
      const std::string& user_agent,
      const scoped_refptr<net::URLRequestContextGetter>& request_context,
      const ResultCallback& callback);
  ~SyncStoppedReporter() override;

#if defined(OPERA_SYNC)
  void SetSyncAccount(const opera::SyncAccount* account);
  void DoCacheAuthHeaderNow();
#endif  // OPERA_SYNC

  // Inform the sync server that sync was stopped on this device.
  // |access_token|, |cache_guid|, and |birthday| must not be empty.
  void ReportSyncStopped(const std::string& access_token,
                         const std::string& cache_guid,
                         const std::string& birthday);

  // net::URLFetcherDelegate implementation.
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // Override the timer's task runner so it can be triggered in tests.
  void SetTimerTaskRunnerForTest(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner);

 private:
  // Convert the base sync URL into the sync event URL.
  static GURL GetSyncEventURL(const GURL& sync_service_url);

  // Callback for a request timing out.
  void OnTimeout();

#if defined(OPERA_SYNC)
  // Note that this is OK as long a the SyncStoppedReporter does not outlive
  // the ProfileSyncService AND the ProfileSyncService owns the account as it
  // does.
  const opera::SyncAccount* account_;

  // The "sync stopped" event will most of the time be sent just after the
  // account has been logged out. We cannot generate the auth header then
  // since the auth data is cleared at that point.
  // In order to authorize the event POST request we need to cache the
  // auth header just before the account is signed out, compare
  // AccountObserver::OnLogoutRequested().
  std::string cached_auth_header_;
#endif  // OPERA_SYNC

  // Handles timing out requests.
  base::OneShotTimer timer_;

  // The URL for the sync server's event RPC.
  const GURL sync_event_url_;

  // The user agent for the browser.
  const std::string user_agent_;

  // Stored to simplify the API; needed for URLFetcher::Create().
  scoped_refptr<net::URLRequestContextGetter> request_context_;

  // The current URLFetcher. Null unless a request is in progress.
  std::unique_ptr<net::URLFetcher> fetcher_;

  // A callback for request completion or timeout.
  ResultCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(SyncStoppedReporter);
};

}  // namespace syncer

#endif  // COMPONENTS_SYNC_DRIVER_SYNC_STOPPED_REPORTER_H_
