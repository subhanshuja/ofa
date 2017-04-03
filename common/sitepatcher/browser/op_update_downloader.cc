// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sitepatcher/browser/op_update_downloader.h"

#include <memory>
#include <string>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "common/sitepatcher/browser/op_site_patcher_config.h"
#include "common/sitepatcher/config.h"
#include "components/prefs/persistent_pref_store.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_status.h"

using content::BrowserThread;
using net::URLRequestContextGetter;

OpUpdateDownloader::OpUpdateDownloader(scoped_refptr<URLRequestContextGetter> request_context,
                                       PersistentPrefStore* prefs,
                                       const char* prefs_key,
                                       base::Value *time_stamp,
                                       const std::string& url,
                                       const base::FilePath &dest,
                                       const OpSitepatcherConfig& config,
                                       Callback callback)
    : request_context_(request_context),
      prefs_(prefs),
      prefs_key_(prefs_key),
      time_stamp_(time_stamp),
      url_(url),
      dest_(dest),
      config_(config),
      callback_(callback),
      url_fetcher_(NULL) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  Start();
};

OpUpdateDownloader::~OpUpdateDownloader() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  delete time_stamp_;
  delete url_fetcher_;
}

void OpUpdateDownloader::UpdatePrefs(base::Value* time_stamp) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  prefs_->SetValue(prefs_key_, base::WrapUnique(time_stamp),
                   WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
}

void OpUpdateDownloader::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  DCHECK(source == url_fetcher_);

  if (source->GetStatus().status() == net::URLRequestStatus::SUCCESS) {
    DLOG(INFO) << "Fetched update";

    base::FilePath path;
    source->GetResponseAsFilePath(true, &path);

    // Add a Ref so callback_ don't destroy us
    this->AddRef();
    if (callback_.Run()) {
      // Take a ref to keep alive over to UI thread
      BrowserThread::PostTask(BrowserThread::UI,
                              FROM_HERE,
                              base::Bind(&OpUpdateDownloader::UpdatePrefs,
                                         this,
                                         time_stamp_));
      time_stamp_ = NULL;
      // Release the extra ref to destroy this
      this->Release();
      return;
    }
    // Release the extra ref
    this->Release();
  } else {
    DLOG(INFO) << "Failed to fetch update, error = " <<
      source->GetStatus().status();
  }

  delete url_fetcher_;
  url_fetcher_ = NULL;

  timer_.Start(FROM_HERE,
               base::TimeDelta::FromSeconds(config_.update_fail_interval),
               this,
               &OpUpdateDownloader::Start);
}

void OpUpdateDownloader::Start() {
  DLOG(INFO) << "Downloading update from " << url_.spec();
  DCHECK(!url_fetcher_);

  url_fetcher_ =
      net::URLFetcher::Create(url_, net::URLFetcher::GET, this).release();
  url_fetcher_->SetRequestContext(request_context_.get());
  url_fetcher_->SaveResponseToFileAtPath(dest_,
      BrowserThread::GetTaskRunnerForThread(BrowserThread::FILE));

  url_fetcher_->Start();
}
