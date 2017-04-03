// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/browser/shell_network_delegate.h
// @final-synchronized

#ifndef CHILL_BROWSER_NET_OPERA_NETWORK_DELEGATE_H_
#define CHILL_BROWSER_NET_OPERA_NETWORK_DELEGATE_H_

#include <stdint.h>

#include <map>
#include <string>

#include "base/compiler_specific.h"
#include "net/base/network_delegate.h"

class GURL;

namespace base {
class Value;
}

namespace opera {

class OperaNetworkDelegate : public net::NetworkDelegate {
 public:
  explicit OperaNetworkDelegate(bool off_the_record);
  virtual ~OperaNetworkDelegate();

  void set_turbo_client_id(std::string id) { turbo_client_id_ = id; }
  void set_turbo_device_id(std::string id) { turbo_device_id_ = id; }
  void set_turbo_default_server(std::string default_server) {
    turbo_default_server_ = default_server;
  }

  // Creates a Value summary of the state of the network session. The caller is
  // responsible for deleting the returned value.
  base::Value* SessionNetworkStatsInfoToValue() const;

 private:
  bool IsCompressedVideo(const net::HttpResponseHeaders* headers);
  // net::NetworkDelegate implementation.
  int OnBeforeURLRequest(net::URLRequest* request,
                         const net::CompletionCallback& callback,
                         GURL* new_url) override;
  int OnBeforeStartTransaction(net::URLRequest* request,
                               const net::CompletionCallback& callback,
                               net::HttpRequestHeaders* headers) override;
  void OnBeforeSendHeaders(net::URLRequest* request,
                           const net::ProxyInfo& proxy_info,
                           const net::ProxyRetryInfoMap& proxy_retry_info,
                           net::HttpRequestHeaders* headers) override;
  void OnStartTransaction(net::URLRequest* request,
                          const net::HttpRequestHeaders& headers) override;
  int OnHeadersReceived(
      net::URLRequest* request,
      const net::CompletionCallback& callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url) override;
  void OnBeforeRedirect(net::URLRequest* request,
                        const GURL& new_location) override;
  void OnResponseStarted(net::URLRequest* request) override;
  void OnNetworkBytesReceived(net::URLRequest* request,
                              int64_t bytes_read) override;
  void OnNetworkBytesSent(net::URLRequest* request,
                          int64_t bytes_sent) override;
  void OnCompleted(net::URLRequest* request, bool started) override;
  void OnURLRequestDestroyed(net::URLRequest* request) override;
  void OnPACScriptError(int line_number,
                        const base::string16& error) override;
  AuthRequiredResponse OnAuthRequired(
      net::URLRequest* request,
      const net::AuthChallengeInfo& auth_info,
      const AuthCallback& callback,
      net::AuthCredentials* credentials) override;
  bool OnCanGetCookies(const net::URLRequest& request,
                       const net::CookieList& cookie_list) override;
  bool OnCanSetCookie(const net::URLRequest& request,
                      const std::string& cookie_line,
                      net::CookieOptions* options) override;
  bool OnCanAccessFile(const net::URLRequest& request,
                       const base::FilePath& path) const override;
  bool OnCanEnablePrivacyMode(
      const GURL& url,
      const GURL& first_party_for_cookies) const override;
  bool OnAreExperimentalCookieFeaturesEnabled() const override;
  void OnTurboSession(bool is_secure,
                      std::string* client_id,
                      std::string* device_id,
                      std::string* mcc,
                      std::string* mnc,
                      std::string* key,
                      std::map<std::string, std::string>* headers)
      override;
  void OnTurboStatistics(uint32_t compressed,
                         uint32_t uncompressed) override;
  std::string OnTurboChooseSession(const GURL& url) override;
  std::string OnTurboGetDefaultServer() override;
  void OnTurboSuggestedServer(const std::string& suggested_server) override;

  bool OnAreStrictSecureCookiesEnabled() const override;

  bool OnCancelURLRequestWithPolicyViolatingReferrerHeader(
      const net::URLRequest& request,
      const GURL& target_url,
      const GURL& referrer_url) const override;

  bool off_the_record_;

  std::string turbo_client_id_;
  std::string turbo_device_id_;
  std::string turbo_default_server_;

  bool on_mobile_network_connection_;

  bool experimental_web_platform_features_enabled_;

  // Total size of all content (excluding headers) that has been received
  // over the network.
  int64_t received_content_length_;

  // Number of compressed videos
  uint32_t videos_compressed_delta_;

  DISALLOW_COPY_AND_ASSIGN(OperaNetworkDelegate);
};

}  // namespace opera

#endif  // CHILL_BROWSER_NET_OPERA_NETWORK_DELEGATE_H_
