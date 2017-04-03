// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SITEPATCHER_BROWSER_OP_UPDATE_DOWNLOADER_H_
#define COMMON_SITEPATCHER_BROWSER_OP_UPDATE_DOWNLOADER_H_

#include <string>

#include "base/files/file_path.h"
#include "base/timer/timer.h"
#include "content/public/browser/browser_thread.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

#include "common/sitepatcher/browser/op_site_patcher_config.h"

namespace base {
class Value;
}

namespace net {
class URLFetcher;
}

class PersistentPrefStore;

class OpUpdateDownloader :
    public base::RefCountedThreadSafe<
  OpUpdateDownloader, content::BrowserThread::DeleteOnFileThread>,
    public net::URLFetcherDelegate {
 public:
  typedef base::Callback<bool(void)> Callback;

  OpUpdateDownloader(scoped_refptr<net::URLRequestContextGetter> request_context,
                     PersistentPrefStore* prefs,
                     const char* prefs_key,
                     base::Value* time_stamp,
                     const std::string& url,
                     const base::FilePath &dest,
                     const OpSitepatcherConfig& config,
                     Callback callback);

  // Inherited from URLFetcherDelegate
  void OnURLFetchComplete(const net::URLFetcher* source) override;

 private:
  ~OpUpdateDownloader() override;
  friend struct content::BrowserThread::DeleteOnThread<
      content::BrowserThread::FILE>;
  friend class base::DeleteHelper<OpUpdateDownloader>;

  void UpdatePrefs(base::Value* time_stamp);
  void Start();

  scoped_refptr<net::URLRequestContextGetter> request_context_;
  PersistentPrefStore* prefs_;
  const char* prefs_key_;
  base::Value* time_stamp_;
  GURL url_;
  base::FilePath dest_;
  const OpSitepatcherConfig& config_;
  Callback callback_;

  net::URLFetcher* url_fetcher_;

  base::OneShotTimer timer_;
};

#endif  // COMMON_SITEPATCHER_BROWSER_OP_UPDATE_DOWNLOADER_H_
