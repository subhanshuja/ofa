// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/shell_url_request_context_getter.cc
// @final-synchronized

#include "chill/browser/net/opera_url_request_context_getter.h"

#include <algorithm>
#include <vector>

#include "base/android/locale_utils.h"
#include "base/android/path_utils.h"
#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/threading/worker_pool.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/cookie_store_factory.h"
#include "content/public/common/content_switches.h"
#include "net/base/cache_type.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/multi_log_ct_verifier.h"
#include "net/dns/host_resolver.h"
#include "net/dns/mapped_host_resolver.h"
#include "net/ftp/ftp_network_layer.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_cache.h"
#include "net/http/http_network_session.h"
#include "net/http/http_server_properties_impl.h"
#include "net/http/http_stream_factory.h"
#include "net/http/http_util.h"
#include "net/http/transport_security_state.h"
#include "net/proxy/proxy_service.h"
#include "net/ssl/channel_id_service.h"
#include "net/ssl/default_channel_id_store.h"
#include "net/ssl/ssl_config_service_defaults.h"
#include "net/turbo/turbo_session_pool.h"
#include "net/url_request/data_protocol_handler.h"
#include "net/url_request/file_protocol_handler.h"
#include "net/url_request/ftp_protocol_handler.h"
#include "net/url_request/static_http_user_agent_settings.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_storage.h"
#include "net/url_request/url_request_intercepting_job_factory.h"
#include "net/url_request/url_request_job_factory_impl.h"

#include "chill/browser/net/opera_ct_policy_enforcer.h"
#include "chill/browser/net/opera_network_delegate.h"
#include "chill/common/opera_content_client.h"

#include "common/settings/settings_manager.h"

using base::FilePath;
using content::BrowserThread;

namespace opera {

OperaURLRequestContextGetter::OperaURLRequestContextGetter(
    const base::FilePath& base_path,
    base::MessageLoop* io_loop,
    base::MessageLoop* file_loop,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors,
    bool off_the_record,
    net::NetLog* net_log)
    : base_path_(base_path),
      io_loop_(io_loop),
      file_loop_(file_loop),
      request_interceptors_(std::move(request_interceptors)),
      off_the_record_(off_the_record),
      turbo_enabled_(false),
      turbo_ad_blocking_enabled_(false),
      turbo_video_compression_enabled_(false),
      turbo_image_quality_(net::TurboImageQuality::MEDIUM),
      net_log_(net_log) {
  // Must first be created on the UI thread.
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  std::swap(protocol_handlers_, *protocol_handlers);

  // We must create the proxy config service on the UI loop on Linux because it
  // must synchronously run on the glib message loop. This will be passed to
  // the URLRequestContextStorage on the IO thread in GetURLRequestContext().
  proxy_config_service_ = net::ProxyService::CreateSystemProxyConfigService(
      io_loop_->task_runner(), file_loop_->task_runner());

  // Enable proxy_for_local_servers to avoid browser resolve
  // destination host name before send http request if http proxy
  // server is configured. Some mobile WAP APN can not resolve any
  // hostname, this cause proxied http connection establish after
  // serval times of failure host resolving.  See CHR-136 and PING-1051.
  net::HttpStreamFactory::set_proxy_for_local_servers_enabled(true);

  if (!off_the_record) {
    turbo_client_id_ = SettingsManager::GetTurboClientId();
    turbo_device_id_ = SettingsManager::GetTurboDeviceId();
    turbo_default_server_ = SettingsManager::GetTurboSuggestedServer();
  }

  turbo_enabled_ = SettingsManager::GetCompression();
  turbo_ad_blocking_enabled_ = SettingsManager::GetAdBlocking();
  turbo_video_compression_enabled_ = SettingsManager::GetVideoCompression();
  turbo_image_quality_ = SettingsManager::GetTurboImageQualityMode();
}

OperaURLRequestContextGetter::~OperaURLRequestContextGetter() {
}

net::URLRequestContext* OperaURLRequestContextGetter::GetURLRequestContext() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  if (!url_request_context_.get()) {
    const base::CommandLine& command_line =
        *base::CommandLine::ForCurrentProcess();

    url_request_context_.reset(new net::URLRequestContext());
    url_request_context_->set_net_log(net_log_);
    OperaNetworkDelegate* opera_network_delegate(
        new OperaNetworkDelegate(off_the_record_));
    network_delegate_.reset(opera_network_delegate);
    opera_network_delegate->set_turbo_client_id(turbo_client_id_);
    opera_network_delegate->set_turbo_device_id(turbo_device_id_);
    opera_network_delegate->set_turbo_default_server(turbo_default_server_);

    net::HttpStreamFactory::set_turbo2_enabled(turbo_enabled_);
    net::TurboSessionPool::SetAdblockBlankEnabled(
        turbo_ad_blocking_enabled_);
    net::TurboSessionPool::SetVideoCompressionEnabled(
        turbo_video_compression_enabled_);
    net::TurboSessionPool::SetImageQuality(turbo_image_quality_);

    url_request_context_->set_network_delegate(network_delegate_.get());
    storage_.reset(
        new net::URLRequestContextStorage(url_request_context_.get()));

    FilePath app_data_folder;
    FilePath cookie_path;

    // Setting an empty cookie path signals an in-memory database
    if (!off_the_record_ &&
        PathService::Get(base::DIR_ANDROID_APP_DATA, &app_data_folder)) {
      cookie_path = app_data_folder.Append(FILE_PATH_LITERAL("cookies"));
    }

    content::CookieStoreConfig cookie_config(
        cookie_path, content::CookieStoreConfig::EPHEMERAL_SESSION_COOKIES,
        NULL, NULL);
    storage_->set_cookie_store(content::CreateCookieStore(cookie_config));

    storage_->set_channel_id_service(base::WrapUnique(
        new net::ChannelIDService(new net::DefaultChannelIDStore(NULL),
                                  base::WorkerPool::GetTaskRunner(true))));

    storage_->set_http_user_agent_settings(std::move(
        base::WrapUnique(new net::StaticHttpUserAgentSettings(
                            GenerateAcceptLanguagesHeader(), GetUserAgent()))));

    std::unique_ptr<net::HostResolver> host_resolver(
        net::HostResolver::CreateDefaultResolver(
            url_request_context_->net_log()));

    storage_->set_cert_verifier(net::CertVerifier::CreateDefault());
    storage_->set_cert_transparency_verifier(
        base::WrapUnique(new net::MultiLogCTVerifier()));
    storage_->set_ct_policy_enforcer(
        base::WrapUnique(new OperaCTPolicyEnforcer()));
    storage_->set_transport_security_state(
        base::WrapUnique(new net::TransportSecurityState));
    // TODO(jam): use v8 if possible, look at chrome code.
    storage_->set_proxy_service(
        net::ProxyService::CreateUsingSystemProxyResolver(
            std::move(proxy_config_service_), 0, NULL));
    storage_->set_ssl_config_service(new net::SSLConfigServiceDefaults);
    storage_->set_http_auth_handler_factory(
        net::HttpAuthHandlerFactory::CreateDefault(host_resolver.get()));
    storage_->set_http_server_properties(std::unique_ptr<net::HttpServerProperties>(
        new net::HttpServerPropertiesImpl()));

    FilePath cache_path;
    CHECK(PathService::Get(base::DIR_CACHE, &cache_path));

    std::unique_ptr<net::HttpCache::BackendFactory> backend_factory;
    if (off_the_record_) {
      backend_factory = net::HttpCache::DefaultBackend::InMemory(0);
    } else {
      backend_factory.reset(new net::HttpCache::DefaultBackend(
          net::DISK_CACHE, net::CACHE_BACKEND_DEFAULT,
          cache_path.Append(FILE_PATH_LITERAL("cache")), 0,
          BrowserThread::GetTaskRunnerForThread(BrowserThread::CACHE)));
    }

    net::HttpNetworkSession::Params network_session_params;
    network_session_params.cert_verifier =
        url_request_context_->cert_verifier();
    network_session_params.ct_policy_enforcer =
        url_request_context_->ct_policy_enforcer();
    network_session_params.cert_transparency_verifier =
        url_request_context_->cert_transparency_verifier();
    network_session_params.transport_security_state =
        url_request_context_->transport_security_state();
    network_session_params.channel_id_service =
        url_request_context_->channel_id_service();
    network_session_params.proxy_service =
        url_request_context_->proxy_service();
    network_session_params.ssl_config_service =
        url_request_context_->ssl_config_service();
    network_session_params.http_auth_handler_factory =
        url_request_context_->http_auth_handler_factory();
    network_session_params.network_delegate =
        url_request_context_->network_delegate();
    network_session_params.http_server_properties =
        url_request_context_->http_server_properties();
    network_session_params.net_log = url_request_context_->net_log();
    network_session_params.ignore_certificate_errors = false;
    if (command_line.HasSwitch(switches::kTestingFixedHttpPort)) {
      int value;
      base::StringToInt(
          command_line.GetSwitchValueASCII(switches::kTestingFixedHttpPort),
          &value);
      network_session_params.testing_fixed_http_port = value;
    }
    if (command_line.HasSwitch(switches::kTestingFixedHttpsPort)) {
      int value;
      base::StringToInt(
          command_line.GetSwitchValueASCII(switches::kTestingFixedHttpsPort),
          &value);
      network_session_params.testing_fixed_https_port = value;
    }
    if (command_line.HasSwitch(switches::kHostResolverRules)) {
      std::unique_ptr<net::MappedHostResolver> mapped_host_resolver(
          new net::MappedHostResolver(std::move(host_resolver)));
      mapped_host_resolver->SetRulesFromString(
          command_line.GetSwitchValueASCII(switches::kHostResolverRules));
      host_resolver = std::move(mapped_host_resolver);
    }

    // Give |storage_| ownership at the end in case it's |mapped_host_resolver|.
    storage_->set_host_resolver(std::move(host_resolver));
    network_session_params.host_resolver =
        url_request_context_->host_resolver();

    storage_->set_http_transaction_factory(base::WrapUnique(
        new net::HttpCache(new net::HttpNetworkSession(network_session_params),
                           std::move(backend_factory), true)));

    // Create the SDCH manager in order to support SDCH HTTP compression. Note
    // that while this object may appear to be unused, it will actually
    // register itself as an available feature upon creation.
    sdch_manager_.reset(new net::SdchManager());

    AddProtocolHandler(url::kDataScheme, new net::DataProtocolHandler);
    AddProtocolHandler(url::kFtpScheme,
                       new net::FtpProtocolHandler(new net::FtpNetworkLayer(
                           url_request_context_->host_resolver())));
    AddProtocolHandler(
        url::kFileScheme,
        new net::FileProtocolHandler(
            content::BrowserThread::GetBlockingPool()
                ->GetTaskRunnerWithShutdownBehavior(
                    base::SequencedWorkerPool::SKIP_ON_SHUTDOWN)));

    InstallProtocolHandlers();
  }

  return url_request_context_.get();
}

scoped_refptr<base::SingleThreadTaskRunner>
OperaURLRequestContextGetter::GetNetworkTaskRunner() const {
  return BrowserThread::GetTaskRunnerForThread(BrowserThread::IO);
}

net::HostResolver* OperaURLRequestContextGetter::host_resolver() {
  return url_request_context_->host_resolver();
}

std::string OperaURLRequestContextGetter::GenerateAcceptLanguagesHeader() {
  const std::string locale = base::android::GetDefaultLocale();
  std::vector<std::string> accept_language_vector;
  accept_language_vector.push_back(locale);

  // For example, if 'zh-TW' was the default, then 'zh' is also a candidate
  size_t pos = locale.find_first_of('-');
  std::string language = locale;
  bool add_en_later = false;
  if (pos != std::string::npos) {
    language = locale.substr(0, pos);

    // use add_en_later flag to ensure "en" is after "en-us".
    add_en_later = (base::ToLowerASCII(language) == "en");
    if (!add_en_later) {
      accept_language_vector.push_back(language);
    }
  }

  // Always let en-us/en as the fallback accept encoding.
  if (base::ToLowerASCII(locale) != "en-us") {
    accept_language_vector.push_back("en-us");
  }

  if (add_en_later || base::ToLowerASCII(language) != "en") {
    accept_language_vector.push_back("en");
  }

  std::string accept_languages = base::JoinString(accept_language_vector, ",");

  // Add q-value
  return net::HttpUtil::GenerateAcceptLanguageHeader(accept_languages);
}

namespace {

void FlushCookiesOnIOThread(scoped_refptr<net::URLRequestContextGetter> urcg) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  net::CookieStore* cookie_store = urcg->GetURLRequestContext()->cookie_store();
  if (cookie_store) {
    cookie_store->FlushStore(base::Closure());
  }
}

}  // namespace

void OperaURLRequestContextGetter::FlushCookieStorage() {
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&FlushCookiesOnIOThread, make_scoped_refptr(this)));
}

void OperaURLRequestContextGetter::OnAppDestroy() {
}

bool OperaURLRequestContextGetter::IsHandledProtocol(
    const std::string& scheme) {
  {
    base::AutoLock lock(installed_protocols_lock_);
    if (installed_protocols_.find(scheme) != installed_protocols_.end())
      return true;
  }

  return net::URLRequest::IsHandledProtocol(scheme);
}

void OperaURLRequestContextGetter::AddProtocolHandler(
    const std::string& scheme,
    net::URLRequestJobFactory::ProtocolHandler* handler) {
  protocol_handlers_[scheme] =
      linked_ptr<net::URLRequestJobFactory::ProtocolHandler>(handler);
}

void OperaURLRequestContextGetter::InstallProtocolHandlers() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  // Store information about installed protocols accessible on any thread
  {
    base::AutoLock lock(installed_protocols_lock_);
    for (content::ProtocolHandlerMap::iterator it = protocol_handlers_.begin();
         it != protocol_handlers_.end();
         ++it)
      installed_protocols_.insert(it->first);
  }

  // Install the handlers
  std::unique_ptr<net::URLRequestJobFactoryImpl> job_factory(
      new net::URLRequestJobFactoryImpl());
  for (content::ProtocolHandlerMap::iterator it =
           protocol_handlers_.begin();
       it != protocol_handlers_.end();
       ++it) {
    bool set_protocol = job_factory->SetProtocolHandler(
        it->first, base::WrapUnique(it->second.release()));
    DCHECK(set_protocol);
  }
  protocol_handlers_.clear();

  // Set up interceptors in the reverse order.
  std::unique_ptr<net::URLRequestJobFactory> top_job_factory = std::move(job_factory);
  for (content::URLRequestInterceptorScopedVector::reverse_iterator i =
           request_interceptors_.rbegin();
       i != request_interceptors_.rend();
       ++i) {
    top_job_factory.reset(new net::URLRequestInterceptingJobFactory(
        std::move(top_job_factory), base::WrapUnique(*i)));
  }
  request_interceptors_.weak_clear();

  storage_->set_job_factory(std::move(top_job_factory));
}

}  // namespace opera
