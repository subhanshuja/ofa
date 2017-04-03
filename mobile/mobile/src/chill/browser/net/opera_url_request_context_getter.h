// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/shell_url_request_context_getter.h
// @final-synchronized

#ifndef CHILL_BROWSER_NET_OPERA_URL_REQUEST_CONTEXT_GETTER_H_
#define CHILL_BROWSER_NET_OPERA_URL_REQUEST_CONTEXT_GETTER_H_

#include <set>
#include <string>

#include "base/compiler_specific.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/browser_context.h"
#include "net/base/sdch_manager.h"
#include "net/turbo/turbo_constants.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_job_factory.h"

namespace base {
class MessageLoop;
}

namespace net {
class HostResolver;
class MappedHostResolver;
class NetLog;
class NetworkDelegate;
class ProxyConfigService;
class URLRequestContextStorage;
}

namespace opera {

class OperaURLRequestContextGetter : public net::URLRequestContextGetter {
 public:
  OperaURLRequestContextGetter(
      const base::FilePath& base_path,
      base::MessageLoop* io_loop,
      base::MessageLoop* file_loop,
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors,
      bool off_the_record,
      net::NetLog* net_log);

  // net::URLRequestContextGetter implementation.
  net::URLRequestContext* GetURLRequestContext() override;
  virtual scoped_refptr<base::SingleThreadTaskRunner>
      GetNetworkTaskRunner() const override;

  net::HostResolver* host_resolver();

  static std::string GenerateAcceptLanguagesHeader();

  void FlushCookieStorage();
  void OnAppDestroy();

  bool IsHandledProtocol(const std::string& scheme);

 protected:
  virtual ~OperaURLRequestContextGetter();
  void AddProtocolHandler(const std::string& scheme,
                          net::URLRequestJobFactory::ProtocolHandler* handler);
  void InstallProtocolHandlers();

 private:
  base::FilePath base_path_;
  base::MessageLoop* io_loop_;
  base::MessageLoop* file_loop_;

  std::unique_ptr<net::ProxyConfigService> proxy_config_service_;
  std::unique_ptr<net::NetworkDelegate> network_delegate_;
  std::unique_ptr<net::URLRequestContextStorage> storage_;
  std::unique_ptr<net::URLRequestContext> url_request_context_;
  content::ProtocolHandlerMap protocol_handlers_;
  content::URLRequestInterceptorScopedVector request_interceptors_;

  // Added by Opera.
  bool off_the_record_;
  std::string turbo_client_id_;
  std::string turbo_device_id_;
  std::string turbo_default_server_;

  bool turbo_enabled_;
  bool turbo_ad_blocking_enabled_;
  bool turbo_video_compression_enabled_;
  net::TurboImageQuality turbo_image_quality_;
  net::NetLog* net_log_;

  base::Lock installed_protocols_lock_;
  std::set<std::string> installed_protocols_;

  std::unique_ptr<net::SdchManager> sdch_manager_;

  DISALLOW_COPY_AND_ASSIGN(OperaURLRequestContextGetter);
};

}  // namespace opera

#endif  // CHILL_BROWSER_NET_OPERA_URL_REQUEST_CONTEXT_GETTER_H_
