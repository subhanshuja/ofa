// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/shell_network_delegate.cc
// @final-synchronized

#include "chill/browser/net/opera_network_delegate.h"

#include <sys/stat.h>
#include <unistd.h>

#include <map>
#include <vector>

#include "base/base64.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/strings/string_number_conversions.h"
#include "content/browser/loader/resource_request_info_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_switches.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"

#include "chill/browser/net/cookie_policy.h"
#include "chill/browser/net/turbo_delegate.h"
#include "chill/browser/net/turbo_zero_rating_delegate.h"
#include "chill/browser/opera_browser_context.h"

namespace {

// Compare real paths by first matching path names and then device id and inode
// number if path names differ.
bool IsSamePath(const base::FilePath& path1, const base::FilePath& path2) {
  if (path1 == path2)
    return true;

  struct stat buf1;
  if (stat(path1.value().c_str(), &buf1))
    return false;

  struct stat buf2;
  if (stat(path2.value().c_str(), &buf2))
    return false;

  return buf1.st_dev == buf2.st_dev && buf1.st_ino == buf2.st_ino;
}

}  // namespace

namespace opera {

OperaNetworkDelegate::OperaNetworkDelegate(bool off_the_record)
    : off_the_record_(off_the_record),
      on_mobile_network_connection_(false),
      experimental_web_platform_features_enabled_(
          base::CommandLine::ForCurrentProcess()->HasSwitch(
              switches::kEnableExperimentalWebPlatformFeatures)),
      received_content_length_(0),
      videos_compressed_delta_(0) {
}

OperaNetworkDelegate::~OperaNetworkDelegate() {
}

base::Value* OperaNetworkDelegate::SessionNetworkStatsInfoToValue() const {
  base::DictionaryValue* dict = new base::DictionaryValue();
  // Use strings to avoid overflow.  base::Value only supports 32-bit integers.
  dict->SetString("session_received_content_length",
                  base::Int64ToString(received_content_length_));
  // TODO(ohrn): Report original content length.
  dict->SetString("session_original_content_length",
                  base::Int64ToString(received_content_length_));
  return dict;
}

int OperaNetworkDelegate::OnBeforeURLRequest(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    GURL* new_url) {
  if (off_the_record_)
    request->SetLoadFlags(request->load_flags() | net::LOAD_BYPASS_TURBO);
  return net::OK;
}

int OperaNetworkDelegate::OnBeforeStartTransaction(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    net::HttpRequestHeaders* headers) {
  return net::OK;
}

void OperaNetworkDelegate::OnBeforeSendHeaders(
    net::URLRequest* request,
    const net::ProxyInfo& proxy_info,
    const net::ProxyRetryInfoMap& proxy_retry_info,
    net::HttpRequestHeaders* headers) {
}

void OperaNetworkDelegate::OnStartTransaction(
    net::URLRequest* request,
    const net::HttpRequestHeaders& headers) {
}

int OperaNetworkDelegate::OnHeadersReceived(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url) {
#ifdef TURBO2_PROXY_HOST_ZERO_RATE_SLOT
  TurboDelegate* turbo_delegate =
      OperaBrowserContext::GetDefaultBrowserContext()->turbo_delegate();

  if (turbo_delegate) {
    turbo_delegate->OnHeadersReceived(request, original_response_headers);
  }
#endif  // TURBO2_PROXY_HOST_ZERO_RATE_SLOT
  if (IsCompressedVideo(original_response_headers)) {
    videos_compressed_delta_++;
  }
  return net::OK;
}

void OperaNetworkDelegate::OnBeforeRedirect(net::URLRequest* request,
                                            const GURL& new_location) {
}

void OperaNetworkDelegate::OnResponseStarted(net::URLRequest* request) {
}

void OperaNetworkDelegate::OnNetworkBytesReceived(
    net::URLRequest* request, int64_t bytes_read) {
}

void OperaNetworkDelegate::OnNetworkBytesSent(net::URLRequest* request,
                                              int64_t bytes_sent) {
}

void OperaNetworkDelegate::OnCompleted(net::URLRequest* request, bool started) {
  if (request->status().status() == net::URLRequestStatus::SUCCESS) {
    int64_t received_content_length = request->received_response_content_length();

    // Only record for http or https urls.
    bool is_http = request->url().SchemeIs("http");
    bool is_https = request->url().SchemeIs("https");

    if (!request->was_cached() &&         // Don't record cached content
        received_content_length &&        // Zero-byte responses aren't useful.
        (is_http || is_https)) {          // Only record for HTTP or HTTPS urls.
      received_content_length_ += received_content_length;
      // TODO(ohrn): Track original content length when turbo is enabled.
    }
  }
}

void OperaNetworkDelegate::OnURLRequestDestroyed(net::URLRequest* request) {
}

void OperaNetworkDelegate::OnPACScriptError(int line_number,
                                            const base::string16& error) {
}

OperaNetworkDelegate::AuthRequiredResponse OperaNetworkDelegate::OnAuthRequired(
    net::URLRequest* request,
    const net::AuthChallengeInfo& auth_info,
    const AuthCallback& callback,
    net::AuthCredentials* credentials) {
  return AUTH_REQUIRED_RESPONSE_NO_ACTION;
}

bool OperaNetworkDelegate::IsCompressedVideo(
    const net::HttpResponseHeaders* headers) {
  return headers->HasHeader("x-opera-video-redirect");
}

bool OperaNetworkDelegate::OnCanGetCookies(const net::URLRequest& request,
                                           const net::CookieList& cookie_list) {
  return CookiePolicy::GetInstance()->CanGetCookies(
      request.url(), request.first_party_for_cookies());
}

bool OperaNetworkDelegate::OnCanSetCookie(const net::URLRequest& request,
                                          const std::string& cookie_line,
                                          net::CookieOptions* options) {
  return CookiePolicy::GetInstance()->CanSetCookie(
      request.url(), request.first_party_for_cookies());
}

bool OperaNetworkDelegate::OnCanAccessFile(const net::URLRequest& request,
                                           const base::FilePath& path) const {
  // Access to files in external storage is allowed.
  base::FilePath external_storage_path;
  PathService::Get(base::DIR_ANDROID_EXTERNAL_STORAGE, &external_storage_path);
  if (external_storage_path.IsParent(path))
    return true;

  // Access to saved pages in application data storage is allowed.
  static const char* kSavedPagesDirectoryName = "saved_pages";

  base::FilePath saved_pages_path;
  PathService::Get(base::DIR_ANDROID_APP_DATA, &saved_pages_path);
  saved_pages_path = saved_pages_path.Append(kSavedPagesDirectoryName);

  // We have to check if the real path matches since /data/data and /data/user/0
  // are the same directory on many devices.
  if (IsSamePath(saved_pages_path, path.DirName()))
    return true;

  // Whitelist of other allowed directories.
  static const char* const kLocalAccessWhiteList[] = {
    "/sdcard",
    "/mnt/sdcard",
    "/storage",
    "/mnt/extSdCard",       // Samsung Galaxy S3/S4.
    "/mnt/external_sd",     // LG Motion 4G.
    "/mnt/ext_card",        // Sony Xperia Z.
    "/mnt/external1",       // Motorola Atrix 4G.
    "/mnt/emmc"             // HTC One V
  };

  for (size_t i = 0; i < arraysize(kLocalAccessWhiteList); ++i) {
    const base::FilePath white_listed_path(kLocalAccessWhiteList[i]);
    if (white_listed_path == path.StripTrailingSeparators() ||
        white_listed_path.IsParent(path)) {
      return true;
    }
  }

  DVLOG(1) << "File access denied - " << path.value().c_str();
  return false;
}

bool OperaNetworkDelegate::OnCanEnablePrivacyMode(
    const GURL& url,
    const GURL& first_party_for_cookies) const {
  bool reading_cookie_allowed = CookiePolicy::GetInstance()->CanGetCookies(
      url, first_party_for_cookies);
  bool setting_cookie_allowed = CookiePolicy::GetInstance()->CanSetCookie(
      url, first_party_for_cookies);
  bool privacy_mode = !(reading_cookie_allowed && setting_cookie_allowed);
  return privacy_mode;
}

bool OperaNetworkDelegate::OnAreExperimentalCookieFeaturesEnabled() const {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kEnableExperimentalWebPlatformFeatures);
}

namespace {

void CallOnTurboClientId(std::string client_id) {
  TurboDelegate* turbo_delegate =
      OperaBrowserContext::GetDefaultBrowserContext()->turbo_delegate();

  if (turbo_delegate)
    turbo_delegate->OnTurboClientId(client_id);
}

void CallOnTurboStatistics(uint32_t compressed, uint32_t uncompressed,
    uint32_t videos_compressed_delta) {
  TurboDelegate* turbo_delegate =
      OperaBrowserContext::GetDefaultBrowserContext()->turbo_delegate();

  if (turbo_delegate) {
    turbo_delegate->OnTurboStatistics(compressed, uncompressed,
                                      videos_compressed_delta);
  }
}

void CallOnTurboSuggestedServer(std::string suggested_server) {
  TurboDelegate* turbo_delegate =
      OperaBrowserContext::GetDefaultBrowserContext()->turbo_delegate();

  if (turbo_delegate)
    turbo_delegate->OnTurboSuggestedServer(suggested_server);
}

}  // namespace

void OperaNetworkDelegate::OnTurboSession(
    bool is_secure,
    std::string* client_id,
    std::string* device_id,
    std::string* mcc,
    std::string* mnc,
    std::string* key,
    std::map<std::string, std::string>* headers) {
  if (turbo_client_id_.empty()) {
    if (!off_the_record_) {
      std::string client_id_base64;
      base::Base64Encode(*client_id, &client_id_base64);
      content::BrowserThread::PostTask(
          content::BrowserThread::UI,
          FROM_HERE,
          base::Bind(&CallOnTurboClientId, client_id_base64));
    }
    turbo_client_id_ = *client_id;
  } else {
    *client_id = turbo_client_id_;
  }

  if (!turbo_device_id_.empty())
    *device_id = turbo_device_id_;

  // Set MCC and MNC if available based on TelephonyManager.getNetworkOperator()
  // For more info about MCC/MNC, see
  // http://en.wikipedia.org/wiki/Mobile_country_code
  TurboDelegate* turbo_delegate =
      OperaBrowserContext::GetDefaultBrowserContext()->turbo_delegate();
  if (turbo_delegate) {
    CountryInformation country_info = turbo_delegate->GetCountryInformation();
    on_mobile_network_connection_ = country_info.is_mobile_connection;

    // Setting MCC/MNC is the "secret" signal to tell the Turbo servers that the
    // connection is over a non-wifi channel
    if (country_info.is_mobile_connection) {
      *mcc = country_info.mobile_country_code;
      *mnc = country_info.mobile_network_code;
    }

    if (is_secure) {
      *key = turbo_delegate->GetPrivateDataKey();
    }

    std::vector<std::string> strings =
        turbo_delegate->GetHelloHeaders(is_secure);
    std::vector<std::string>::const_iterator it;
    for (it = strings.begin(); it != strings.end();) {
      std::string key = *it++;
      (*headers)[key] = *it++;
    }
  }
}

void OperaNetworkDelegate::OnTurboStatistics(
    uint32_t compressed, uint32_t uncompressed) {
  content::BrowserThread::PostTask(
      content::BrowserThread::UI,
      FROM_HERE,
      base::Bind(&CallOnTurboStatistics, compressed, uncompressed,
          videos_compressed_delta_));
  videos_compressed_delta_ = 0;
}

std::string OperaNetworkDelegate::OnTurboChooseSession(const GURL& url) {
#ifdef TURBO2_PROXY_HOST_ZERO_RATE_SLOT
  turbo::zero_rating::Delegate* delegate = turbo::zero_rating::Delegate::get();
  if (on_mobile_network_connection_ && delegate) {
    int cluster_nr = delegate->MatchUrl(url.spec().c_str());

    // 0 is magic and means that we should not zero-rate
    if (cluster_nr <= 0)
      return "";

    return base::StringPrintf(TURBO2_PROXY_HOST_ZERO_RATE_SLOT, cluster_nr);
  }
#endif  // TURBO2_PROXY_HOST_ZERO_RATE_SLOT
  return "";
}

std::string OperaNetworkDelegate::OnTurboGetDefaultServer() {
  return turbo_default_server_;
}

void OperaNetworkDelegate::OnTurboSuggestedServer(
    const std::string& suggested_server) {
  turbo_default_server_ = suggested_server;
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&CallOnTurboSuggestedServer, suggested_server));
}

bool OperaNetworkDelegate::OnAreStrictSecureCookiesEnabled() const {
  return OnAreExperimentalCookieFeaturesEnabled();
}

bool OperaNetworkDelegate::OnCancelURLRequestWithPolicyViolatingReferrerHeader(
    const net::URLRequest& request,
    const GURL& target_url,
    const GURL& referrer_url) const {
  return true;
}

}  // namespace opera
