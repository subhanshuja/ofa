// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sitepatcher/browser/op_update_checker.h"

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/format_macros.h"
#include "base/json/json_reader.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "common/sitepatcher/browser/op_site_patcher.h"
#include "common/sitepatcher/browser/op_update_downloader.h"
#include "common/sitepatcher/config.h"
#include "components/prefs/persistent_pref_store.h"
#include "content/public/browser/browser_thread.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_status.h"
#include "url/gurl.h"

using base::Value;
using content::BrowserThread;
using net::URLFetcher;

#define GET_DOUBLE(key, dest) \
  (prefs_->GetValue(key, &value) && value->GetAsDouble(&dest))

class OpUpdateCheckerImpl : public net::URLFetcherDelegate {
  friend class OpUpdateChecker;
  OpUpdateCheckerImpl(
      scoped_refptr<net::URLRequestContextGetter> request_context,
      const OpSitepatcherConfig& config,
      OpSitePatcher* parent)
      : prefs_(NULL),
        url_fetcher_(NULL),
        request_context_(request_context),
        config_(config),
        parent_(parent) {}

  ~OpUpdateCheckerImpl() override {
    content::BrowserThread::DeleteSoon(content::BrowserThread::UI, FROM_HERE,
                                       url_fetcher_);
  }

  // Inherited from URLFetcherDelegate
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  void Start(bool force_check);
  void Timeout();  // Called by timer.
  void NewTime(double time_to_next_check);
  double TimeSinceLastCheck();
  bool CheckForExistingUpdates();
  bool DownloadIfOutOfDate(double prefs_override_server,
                           double browserjs_server,
                           double webbt_server);

  bool DownloadUpdates(double prefs_overide_server,
                       double browser_js_server,
                       double webbt_server);
  void DownloadUpdatesOnFileThread(double prefs_overide_server,
                                   double browser_js_server,
                                   double webbt_server,
                                   scoped_refptr<OpSitePatcher> patcher);

  bool ParseMetadata(const std::string& data,
                     double* spoof,
                     double* bspit,
                     double* btblacklist);
  void UpdatePrefs(double now, double spoof, double bspit, double btblacklist);

  bool PrefsOverrideCallback(scoped_refptr<OpSitePatcher> patcher);
  bool BrowserJSCallback(scoped_refptr<OpSitePatcher> patcher);
  bool WebBluetoothBlacklistCallback(scoped_refptr<OpSitePatcher> patcher);

  PersistentPrefStore* prefs_;

  net::URLFetcher* url_fetcher_;
  scoped_refptr<net::URLRequestContextGetter> request_context_;
  const OpSitepatcherConfig& config_;
  OpSitePatcher* parent_;

  scoped_refptr<OpUpdateDownloader> po_downloader_;
  scoped_refptr<OpUpdateDownloader> bjs_downloader_;
  scoped_refptr<OpUpdateDownloader> webbt_downloader_;

  base::OneShotTimer timer_;
};

OpUpdateChecker::OpUpdateChecker(
    scoped_refptr<net::URLRequestContextGetter> request_context,
    const OpSitepatcherConfig& config,
    OpSitePatcher* parent)
    : impl_(new OpUpdateCheckerImpl(request_context, config, parent)) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
}

OpUpdateChecker::~OpUpdateChecker() {
  delete impl_;
}

void OpUpdateChecker::SetPrefs(PersistentPrefStore* prefs) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  impl_->prefs_ = prefs;
}

void OpUpdateChecker::Start(bool force_check) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  impl_->Start(force_check);
}

void OpUpdateCheckerImpl::Timeout() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  Start(false);
}

void OpUpdateCheckerImpl::NewTime(double time_to_next_check) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  // Need to always start timer on UI thread as it will be destroyed
  // on the UI thread.
  timer_.Start(FROM_HERE, base::TimeDelta::FromSeconds(time_to_next_check),
               this, &OpUpdateCheckerImpl::Timeout);
}

void OpUpdateCheckerImpl::Start(bool force_check) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(prefs_);

  double time_since_last_check = TimeSinceLastCheck();

  // If there's an outstanding request then the reply from that (or failure to
  // load) will restart the state machine.
  if (url_fetcher_)
    return;

  if (force_check || time_since_last_check == 0 ||
      time_since_last_check >= config_.update_check_interval) {
    std::string url(config_.update_check_url);
    base::StringAppendF(&url, "&timesincelastcheck=%.0f",
                        time_since_last_check);

    DLOG(INFO) << "Starting json fetch, time since last check = "
               << time_since_last_check << " seconds, url = " << url;

    url_fetcher_ =
        URLFetcher::Create(GURL(url), URLFetcher::GET, this).release();
    url_fetcher_->SetRequestContext(request_context_.get());
    url_fetcher_->Start();

  } else {
    double time_to_next_check =
        config_.update_check_interval - time_since_last_check;
    if (time_to_next_check < 0.0)
      time_to_next_check = 0.0;

    if (time_to_next_check > config_.update_fail_interval)
      CheckForExistingUpdates();

    DLOG(INFO) << "Next json fetch in = " << time_to_next_check << " seconds";

    NewTime(time_to_next_check);
  }
}

void OpUpdateCheckerImpl::UpdatePrefs(double now,
                                      double spoof,
                                      double bspit,
                                      double btblacklist) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  prefs_->SetValue(PREF_LAST_CHECK,
                   base::WrapUnique(new base::FundamentalValue(now)),
                   WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);

  prefs_->SetValue(PREF_PREFS_OVERRIDE_SERVER,
                   base::WrapUnique(new base::FundamentalValue(spoof)),
                   WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);

  prefs_->SetValue(PREF_BROWSER_JS_SERVER,
                   base::WrapUnique(new base::FundamentalValue(bspit)),
                   WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);

  prefs_->SetValue(PREF_WEB_BLUETOOTH_BLACKLIST_SERVER,
                   base::WrapUnique(new base::FundamentalValue(btblacklist)),
                   WriteablePrefStore::DEFAULT_PREF_WRITE_FLAGS);
}

void OpUpdateCheckerImpl::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(source == url_fetcher_);

  double next_check = config_.update_fail_interval;

  if (source->GetStatus().status() == net::URLRequestStatus::SUCCESS) {
    DLOG(INFO) << "Fetched update json";

    std::string update_json;

    if (source->GetResponseAsString(&update_json)) {
      DLOG(INFO) << update_json;

      std::string time;
      double spoof = 0, bspit = 0, btblacklist = 0;

      if (ParseMetadata(update_json, &spoof, &bspit, &btblacklist)) {
        DLOG(INFO) << "spoof = " << spoof << ", bspit = " << bspit
                   << ", btblacklist = " << btblacklist;

        double now = base::Time::Now().ToDoubleT();
        UpdatePrefs(now, spoof, bspit, btblacklist);

        DownloadIfOutOfDate(spoof, bspit, btblacklist);

        next_check = config_.update_check_interval;
      }
    } else {
      DLOG(INFO) << "Failed to parse update json - incorrect format?";
    }
  } else {
    DLOG(INFO) << "Failed to fetch update json, error = "
               << source->GetStatus().status();
  }

  delete url_fetcher_;
  url_fetcher_ = NULL;

  NewTime(next_check);
}

double OpUpdateCheckerImpl::TimeSinceLastCheck() {
  double last_check = 0, elapsed = 0;
  const Value* value = NULL;

  if (GET_DOUBLE(PREF_LAST_CHECK, last_check) && last_check > 0) {
    elapsed = base::Time::Now().ToDoubleT() - last_check;

    if (elapsed < 0)
      elapsed = 0;
  }

  return elapsed;
}

bool OpUpdateCheckerImpl::CheckForExistingUpdates() {
  const Value* value = NULL;
  double prefs_override_server = 0;
  double browser_js_server = 0;
  double web_bt_blacklist_server = 0;

  if (GET_DOUBLE(PREF_PREFS_OVERRIDE_SERVER, prefs_override_server) |  // Yes, |
      GET_DOUBLE(PREF_BROWSER_JS_SERVER,
                 browser_js_server) |  // Yes, | here as well
      GET_DOUBLE(PREF_WEB_BLUETOOTH_BLACKLIST_SERVER,
                 web_bt_blacklist_server)) {
    return DownloadIfOutOfDate(prefs_override_server, browser_js_server,
                               web_bt_blacklist_server);
  }

  return false;
}

bool OpUpdateCheckerImpl::DownloadIfOutOfDate(double prefs_override_server,
                                              double browser_js_server,
                                              double web_bt_blacklist_server) {
  const Value* value = NULL;
  double local = 0;
  double prefs_override = 0;
  double browser_js = 0;
  double webbt_blacklist = 0;

  if (!GET_DOUBLE(PREF_PREFS_OVERRIDE_LOCAL, local) ||
      prefs_override_server > local) {
    prefs_override = prefs_override_server;
  }

  if (!GET_DOUBLE(PREF_BROWSER_JS_LOCAL, local) || browser_js_server > local) {
    browser_js = browser_js_server;
  }

  if (!GET_DOUBLE(PREF_WEB_BLUETOOTH_BLACKLIST_LOCAL, local) ||
      web_bt_blacklist_server > local) {
    webbt_blacklist = web_bt_blacklist_server;
  }

  return DownloadUpdates(prefs_override, browser_js, webbt_blacklist);
}

bool OpUpdateCheckerImpl::ParseMetadata(const std::string& data,
                                        double* spoof,
                                        double* bspit,
                                        double* btblacklist) {
  std::unique_ptr<Value> metadata(base::JSONReader::Read(data));
  if (!metadata.get())
    return false;

  base::DictionaryValue* dict = NULL;
  if (!metadata->GetAsDictionary(&dict))
    return false;

  std::string spoof_string;
  if (!dict->GetString("versioncheck.@spoof", &spoof_string))
    return false;
  base::StringToDouble(spoof_string, spoof);

  std::string bspit_string;
  if (!dict->GetString("versioncheck.@bspit", &bspit_string))
    return false;
  base::StringToDouble(bspit_string, bspit);

  std::string btblacklist_string;
  if (!dict->GetString("versioncheck.@btblacklist", &btblacklist_string))
    return false;
  base::StringToDouble(btblacklist_string, btblacklist);

  return true;
}

bool OpUpdateCheckerImpl::DownloadUpdates(double prefs_overide_server,
                                          double browser_js_server,
                                          double webbt_server) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DLOG(INFO) << "Downloading updates, prefs_overide_server = "
             << prefs_overide_server
             << ", browser_js_server = " << browser_js_server
             << ", webbt_server = " << webbt_server;

  if (config_.prefs_override_url.empty())
    prefs_overide_server = 0;
  if (prefs_overide_server <= 0 && browser_js_server <= 0 && webbt_server <= 0)
    return false;

  BrowserThread::PostTask(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(&OpUpdateCheckerImpl::DownloadUpdatesOnFileThread,
                 base::Unretained(this), prefs_overide_server,
                 browser_js_server, webbt_server,
                 scoped_refptr<OpSitePatcher>(parent_)));

  return true;
}

void OpUpdateCheckerImpl::DownloadUpdatesOnFileThread(
    double prefs_overide_server,
    double browser_js_server,
    double webbt_server,
    scoped_refptr<OpSitePatcher> patcher) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  if (prefs_overide_server > 0) {
    // Passing |this| as Unretained is safe here as the instance is kept alive
    // as a sub-object of OpSitePatcher which is passed with scoped_refptr.
    OpUpdateDownloader::Callback callback =
        base::Bind(&OpUpdateCheckerImpl::PrefsOverrideCallback,
                   base::Unretained(this), patcher);
    po_downloader_ = new OpUpdateDownloader(
        request_context_, prefs_, PREF_PREFS_OVERRIDE_LOCAL,
        new base::FundamentalValue(prefs_overide_server),
        config_.prefs_override_url,
        config_.prefs_override_file.AddExtension(FILE_PATH_LITERAL(".new")),
        config_, callback);
  }

  if (browser_js_server > 0) {
    // Passing |this| as Unretained is safe here as the instance is kept alive
    // as a sub-object of OpSitePatcher which is passed with scoped_refptr.
    OpUpdateDownloader::Callback callback =
        base::Bind(&OpUpdateCheckerImpl::BrowserJSCallback,
                   base::Unretained(this), patcher);
    bjs_downloader_ = new OpUpdateDownloader(
        request_context_, prefs_, PREF_BROWSER_JS_LOCAL,
        new base::FundamentalValue(browser_js_server), config_.browser_js_url,
        config_.browser_js_file.AddExtension(FILE_PATH_LITERAL(".new")),
        config_, callback);
  }

  if (webbt_server > 0) {
    // Passing |this| as Unretained is safe here as the instance is kept alive
    // as a sub-object of OpSitePatcher which is passed with scoped_refptr.
    OpUpdateDownloader::Callback callback =
        base::Bind(&OpUpdateCheckerImpl::WebBluetoothBlacklistCallback,
                   base::Unretained(this), patcher);
    webbt_downloader_ = new OpUpdateDownloader(
        request_context_, prefs_, PREF_WEB_BLUETOOTH_BLACKLIST_LOCAL,
        new base::FundamentalValue(webbt_server),
        config_.web_bluetooth_blacklist_url,
        config_.web_bluetooth_blacklist_file.AddExtension(
            FILE_PATH_LITERAL(".new")),
        config_, callback);
  }
}

bool OpUpdateCheckerImpl::PrefsOverrideCallback(
    scoped_refptr<OpSitePatcher> patcher) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  bool success = parent_->OnPrefsOverrideUpdated();
  if (success)
    po_downloader_ = NULL;

  return success;
}

bool OpUpdateCheckerImpl::BrowserJSCallback(
    scoped_refptr<OpSitePatcher> patcher) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  bool success = parent_->OnBrowserJSUpdated();
  if (success)
    bjs_downloader_ = NULL;

  return success;
}

bool OpUpdateCheckerImpl::WebBluetoothBlacklistCallback(
    scoped_refptr<OpSitePatcher> patcher) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  bool success = parent_->OnWebBluetoothBlacklistUpdated();
  if (success)
    webbt_downloader_ = NULL;

  return success;
}
