// Copyright (c) 2015 Opera Software ASA. All rights reserved.

#include "net/turbo/turbo_session_pool.h"

#include <limits>

#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"

#include "net/base/network_delegate.h"
#include "net/http/http_network_session.h"
#include "net/http/http_stream_factory.h"
#include "net/spdy/spdy_session.h"
#include "net/turbo/turbo_cache_synchronizer.h"
#include "net/turbo/turbo_cookie_synchronizer.h"

namespace net {

// static
int TurboSessionPool::configured_image_quality_ = -1;
// static
uint32_t TurboSessionPool::configured_runtime_features_ = 0;
// static
bool TurboSessionPool::configured_server_side_log_ = false;
// static
uint8_t TurboSessionPool::kept_usage_stats_version_ = 1;
// static
base::LazyInstance<std::string> TurboSessionPool::kept_usage_stats_ =
    LAZY_INSTANCE_INITIALIZER;

namespace {

// The number of connections to the default turbo2 proxy.
const int kParallelSessionsCount = 3;
// After processing a SUGGESTED_SERVER turbo2 frame, ignore all subsequent
// SUGGESTED_SERVER frames for this long.
const int kSuggestedServerCooldownMinutes = 10;

bool IsTurboEnabled() {
  return HttpStreamFactory::turbo2_enabled();
}

void DeleteHttpStreamRequest(HttpStreamRequest* request) {
  delete request;
}

}  // namespace

TurboSessionPool::TurboSessionPool(HttpNetworkSession* session)
    : session_(session),
      was_turbo_enabled_(false),
      proxy_session_selector_(1),
      proxy_connection_id_counter_(1),
      suggested_server_cooldown_(false),
      sent_image_quality_(-1),
      sent_server_side_log_(false),
      sent_usage_stats_(""),
      weak_ptr_factory_(this) {
  session->spdy_session_pool()->AddObserver(this);

  turbo_cookie_synchronizer_.reset(new TurboCookieSynchronizer(AsWeakPtr()));
  turbo_cache_synchronizer_.reset(new TurboCacheSynchronizer(AsWeakPtr()));

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch("turbo-image-quality")) {
    std::string value =
        command_line->GetSwitchValueASCII("turbo-image-quality");
    base::StringToInt(value, &configured_image_quality_);
  }

  if (command_line->HasSwitch("turbo-enable-server-log")) {
    SetServerSideLog(true);
  }

  std::string turbo1_proxy = TURBO_PROXY;
  if (command_line->HasSwitch("turbo-proxy")) {
    turbo1_proxy = command_line->GetSwitchValueASCII("turbo-proxy");
  }

  turbo1_proxy_ =
      net::ProxyServer::FromURI(turbo1_proxy, net::ProxyServer::SCHEME_HTTP);
  turbo1_proxy_.set_turbo_connection_id(1);
  compile_time_turbo2_proxy_ =
      net::ProxyServer::FromURI(TURBO2_PROXY, net::ProxyServer::SCHEME_HTTP);
  compile_time_turbo2_proxy_.set_turbo_connection_id(1);

#if defined(TURBO2_CONNECT_ON_STARTUP)
  base::MessageLoop::current()->PostTask(
      FROM_HERE, base::Bind(&TurboSessionPool::ConnectToProxy, AsWeakPtr()));
#endif
}

void TurboSessionPool::CheckTurboEnabled() {
  if (IsTurboEnabled()) {
    if (!was_turbo_enabled_)
      current_loads_.clear();

    was_turbo_enabled_ = true;

    turbo_cache_synchronizer_->TriggerSend();

    if (sent_image_quality_ != configured_image_quality_)
      UpdateImageQuality();

    if (sent_runtime_features_ != configured_runtime_features_)
      UpdateRuntimeFeatures();

    if (sent_server_side_log_ != configured_server_side_log_)
      UpdateServerSideLog();

    if (sent_usage_stats_ != kept_usage_stats_.Get()) {
      UpdateUsageStats();
    }
  } else {
    if (was_turbo_enabled_)
      current_loads_.clear();

    was_turbo_enabled_ = false;
    turbo_cache_synchronizer_->ResetSyncState();
  }
}

void TurboSessionPool::ReportUnclaimedPushedStreams() {
  for (ProxySessionList::iterator session_it = proxy_sessions_.begin();
       session_it != proxy_sessions_.end(); ++session_it) {
    (*session_it)->TurboForceSendUnclaimedStreams();
  }
}

ProxyServer TurboSessionPool::GetProxyServerForRequest(
    const HttpRequestInfo& request_info) {
  if (HttpStreamFactory::turbo1_enabled()) {
    if (request_info.url.SchemeIs("http")) {
      return turbo1_proxy_;
    } else {
      return ProxyServer::Direct();
    }
  } else {
#ifndef TURBO2_CONNECT_ON_STARTUP
    DCHECK(HttpStreamFactory::turbo2_enabled());
#endif
    if (turbo2_proxies_.empty()) {
      // |turbo2_proxies_| needs to be initialized lazily on first call to
      // GetProxyServerForRequest to avoid crashing chromium net_unittests, in
      // which we cannot query the network delegate on startup.
      InitializeTurbo2Proxies();
    }
    // Let zero-rating rules override the normal Turbo behavior
    std::string zero_rating_server;
    zero_rating_server = session_->network_delegate()->NotifyTurboChooseSession(
            request_info.url);
    if (!zero_rating_server.empty()) {
      ProxyServer proxy(
          turbo2_proxies_[0].scheme(),
          HostPortPair(zero_rating_server,
                       turbo2_proxies_[0].host_port_pair().port()));
      // For zero-rating servers we don't use multiple connections. Simply
      // mark it as a turbo proxy.
      proxy.set_turbo_connection_id(1);
      return proxy;
    }

    // Special-case: a single connection.  Keeps the rest simpler.
    if (turbo2_proxies_.size() == 1)
      return turbo2_proxies_[0];
    // If we have a matching pushed stream in any connection, use that
    // connection.
    for (ProxySessionList::iterator it = proxy_sessions_.begin();
         it != proxy_sessions_.end(); ++it) {
      SpdySession* proxy_session = *it;
      if (proxy_session->HasPushStream(request_info.url)) {
        const SpdySessionKey& key = proxy_session->spdy_session_key();
        ProxyServer proxy(turbo2_proxies_[0].scheme(), key.host_port_pair());
        proxy.set_turbo_connection_id(key.unique_id());
        return proxy;
      }
    }
    // Special-case: main frame and HTTPS requests go to first connection.
    if (request_info.turbo_request_type == "main-frame" ||
        request_info.url.SchemeIs("https"))
      return turbo2_proxies_[0];
    // Other requests go to other connections.
    if (turbo2_proxies_.size() <= proxy_session_selector_)
      proxy_session_selector_ = 1;
    return turbo2_proxies_[proxy_session_selector_++];
  }
}

void TurboSessionPool::SendUnclaimed() {
  // The page has been loaded successfully, so the list of the
  // unclaimed data can be sent back to the server.
  if (HasProxySession())
    GetProxySession()->UnclaimedDataCanBeSent();
}

void TurboSessionPool::SignalLoadStart(uintptr_t context, const GURL& url) {
  // Only report HTTP URLs, since Turbo is only used for HTTP.
  if (!url.SchemeIs("http"))
    return;

  // Reset stop time. This will abort any queued SendPageLoadTime calls.
  LoadInfoIterator iter = current_loads_.find(context);
  if (iter != current_loads_.end())
    iter->second.SetStopTime(base::Time());
}

void TurboSessionPool::SignalProvisionalLoadStart(uintptr_t context,
                                                  const GURL& url) {
  // Only report HTTP URLs, since Turbo is only used for HTTP.
  if (!url.SchemeIs("http"))
    return;

  CheckTurboEnabled();

  current_loads_[context] = LoadInfo(base::Time::Now());
}

void TurboSessionPool::SignalLoadRedirect(uintptr_t context,
                                          const GURL& source_url,
                                          const GURL& target_url) {
  // Only report HTTP URLs, since Turbo is only used for HTTP.
  if (!source_url.SchemeIs("http"))
    return;

  CheckTurboEnabled();

  LoadInfoIterator iter = current_loads_.find(context);

  if (iter != current_loads_.end()) {
    // If target URL is not HTTP (and thus shouldn't be reported) or if the
    // redirect is "cross-site", ignore this load from now on.  Our cross-site
    // detection is rather simplistic; it's mostly meant to allow redirects from
    // "<domain>" to "www.<domain>".
    if (!target_url.SchemeIs("http") ||
        (source_url.host() != target_url.host() &&
         !target_url.DomainIs(source_url.host())))
      current_loads_.erase(iter);
  }
}

void TurboSessionPool::SignalLoadStop(uintptr_t context, const GURL& url) {
  LoadInfoIterator iter = current_loads_.find(context);

  if (iter != current_loads_.end()) {
    iter->second.SetStopTime(base::Time::Now());

    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, base::Bind(&TurboSessionPool::SendPageLoadTime, AsWeakPtr(),
                              context, url),
        base::TimeDelta::FromSeconds(2));
  }
}

void TurboSessionPool::SignalConnectionFailed() {
  if (HttpStreamFactory::turbo1_enabled())
    return;

  CHECK(!turbo2_proxies_.empty());
  // If we have some ongoing sessions to the current default proxy, it should
  // still be reachable, so we ignore the error for now.
  bool have_ongoing_session = false;
  HostPortPair current_default = turbo2_proxies_[0].host_port_pair();
  for (ProxySessionList::const_iterator iter = proxy_sessions_.begin();
       iter != proxy_sessions_.end(); ++iter) {
    if (current_default.Equals((*iter)->host_port_pair())) {
      have_ongoing_session = true;
      break;
    }
  }
  if (have_ongoing_session)
    return;

  // Otherwise, reset any suggested server and switch to the default address.
  session_->network_delegate()->NotifyTurboSuggestedServer("");
  InitializeTurbo2Proxies();
}

// static
void TurboSessionPool::SetImageQuality(TurboImageQuality quality) {
  configured_image_quality_ = static_cast<int>(quality);
}

// static
void TurboSessionPool::SetVideoCompressionEnabled(bool enable) {
  SetRuntimeFeature(TURBO_FEATURE_VIDEO_COMPRESSION, enable);
}

// static
void TurboSessionPool::SetAdblockBlankEnabled(bool enable) {
  SetRuntimeFeature(TURBO_FEATURE_ADBLOCK_BLANK, enable);
}

// static
void TurboSessionPool::SetCompresSsl(bool enable) {
  SetRuntimeFeature(TURBO_FEATURE_HTTPS_REQUESTS, enable);
}

// static
void TurboSessionPool::SetServerSideLog(bool enable) {
  configured_server_side_log_ = enable;
}

// static
void TurboSessionPool::SetUsageStats(const std::string& stats,
                                     uint8_t stats_version) {
  kept_usage_stats_.Get() = stats;
  kept_usage_stats_version_ = stats_version;
}

TurboSessionPool::~TurboSessionPool() {
  std::for_each(stream_requests_.begin(), stream_requests_.end(),
                DeleteHttpStreamRequest);
}

void TurboSessionPool::OnTurboSpdySessionCreated(
    const base::WeakPtr<SpdySession> proxy_session) {
  proxy_sessions_.push_back(proxy_session.get());
  proxy_session->AddObserver(this);
}

void TurboSessionPool::OnTurboSpdySessionInitialized(
    const base::WeakPtr<SpdySession> proxy_session) {
  proxy_session->SendImageQuality(configured_image_quality_);
  proxy_session->SendServerSideLog(configured_server_side_log_);
  proxy_session->SendUsageStats(kept_usage_stats_.Get(),
                                kept_usage_stats_version_);
  bool first = proxy_sessions_.front() == proxy_session.get();
  if (first)
    turbo_cache_synchronizer_->OnFirstTurboSession();
}

void TurboSessionPool::OnTurboSpdySessionDestroyed(
    const base::WeakPtr<SpdySession> proxy_session) {
  proxy_session->RemoveObserver(this);

  for (ProxySessionList::iterator it = proxy_sessions_.begin();
       it != proxy_sessions_.end(); ++it) {
    if (*it == proxy_session.get()) {
      proxy_sessions_.erase(it);
      break;
    }
  }

  if (!HasProxySession()) {
    // Clear current loads, so that loads spanning across possible network
    // outages, or similar, are not reported to the proxy.
    current_loads_.clear();
    turbo_cache_synchronizer_->OnTurboConnectionLost();
  }
}

void TurboSessionPool::OnSynStream(const GURL& gurl) {
  turbo_cache_synchronizer_->AddSynchronizedURL(gurl);
}

void TurboSessionPool::OnSynReply(const GURL& gurl) {
  turbo_cache_synchronizer_->AddSynchronizedURL(gurl);
}

void TurboSessionPool::OnTurboSession(
    bool is_secure,
    std::string* client_id,
    std::string* device_id,
    std::string* mcc,
    std::string* mnc,
    std::string* key,
    std::map<std::string, std::string>* headers) {
  session_->network_delegate()->NotifyTurboSession(
      is_secure, client_id, device_id, mcc, mnc, key, headers);
}

void TurboSessionPool::OnTurboStatistics(uint32_t compressed,
                                         uint32_t uncompressed) {
  session_->network_delegate()->NotifyTurboStatistics(compressed, uncompressed);
}

void TurboSessionPool::OnTurboSuggestedServer(
    const std::string& suggested_server) {
  DCHECK(!HttpStreamFactory::turbo1_enabled());
  if (suggested_server.size() > 0) {
    if (suggested_server_cooldown_) {
      return;
    }
    suggested_server_cooldown_ = true;
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, base::Bind(&TurboSessionPool::ResetSuggestedServerCooldown,
                              AsWeakPtr()),
        base::TimeDelta::FromMinutes(kSuggestedServerCooldownMinutes));
    session_->network_delegate()->NotifyTurboSuggestedServer(suggested_server);

    InitializeTurbo2Proxies();
    return;
  }
  LOG(WARNING) << "Turbo2 SUGGESTED_SERVER throttling not implemented.";
}

// Empty implementation of HttpStreamRequest::Delegate.  We don't really care
// about failures here right now.
class TurboStreamRequestDelegate : public HttpStreamRequest::Delegate {
 public:
  explicit TurboStreamRequestDelegate(
      base::WeakPtr<TurboSessionPool> session_pool,
      int num_requests)
      : session_pool_(session_pool), num_requests_(num_requests) {}

 private:
  void OnStreamReady(const SSLConfig& used_ssl_config,
                     const ProxyInfo& used_proxy_info,
                     HttpStream* stream) override;

  void OnWebSocketHandshakeStreamReady(
      const SSLConfig& used_ssl_config,
      const ProxyInfo& used_proxy_info,
      WebSocketHandshakeStreamBase* stream) override {}

  void OnBidirectionalStreamImplReady(
      const SSLConfig& used_ssl_config,
      const ProxyInfo& used_proxy_info,
      BidirectionalStreamImpl* stream) override {}

  void OnStreamFailed(int status,
                      const SSLConfig& used_ssl_config) override {}

  void OnCertificateError(int status,
                          const SSLConfig& used_ssl_config,
                          const SSLInfo& ssl_info) override {}

  void OnNeedsProxyAuth(const HttpResponseInfo& proxy_response,
                        const SSLConfig& used_ssl_config,
                        const ProxyInfo& used_proxy_info,
                        HttpAuthController* auth_controller) override {}

  void OnNeedsClientAuth(const SSLConfig& used_ssl_config,
                         SSLCertRequestInfo* cert_info) override {}

  void OnQuicBroken() override {}

  void OnHttpsProxyTunnelResponse(const HttpResponseInfo& response_info,
                                  const SSLConfig& used_ssl_config,
                                  const ProxyInfo& used_proxy_info,
                                  HttpStream* stream) override {}

  base::WeakPtr<TurboSessionPool> session_pool_;
  int num_requests_;
};

void TurboStreamRequestDelegate::OnStreamReady(const SSLConfig& used_ssl_config,
                                               const ProxyInfo& used_proxy_info,
                                               HttpStream* stream) {
  delete stream;

  if (--num_requests_ == 0) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(&TurboSessionPool::CleanUpStreamRequest, session_pool_));
  }
}

void TurboSessionPool::ConnectToProxy() {
  int num_requests = kParallelSessionsCount;

  stream_request_delegate_.reset(
      new TurboStreamRequestDelegate(AsWeakPtr(), num_requests));

  HttpRequestInfo info;
  info.url = GURL("http://turbo.proxy/");
  info.method = "GET";
  info.load_flags = LOAD_FORCE_TURBO;

  SSLConfig ssl_config;

  while (num_requests--) {
    stream_requests_.push_back(session_->http_stream_factory()->RequestStream(
        info, IDLE, ssl_config, ssl_config, stream_request_delegate_.get(),
        NetLogWithSource::Make(session_->net_log(), NetLogSourceType::NONE)));
  }
}

void TurboSessionPool::CleanUpStreamRequest() {
  std::for_each(stream_requests_.begin(), stream_requests_.end(),
                DeleteHttpStreamRequest);
  stream_requests_.clear();
  stream_request_delegate_.reset();
}

// static
void TurboSessionPool::SetRuntimeFeature(uint32_t feature, bool enable) {
  if (enable) {
    configured_runtime_features_ |= feature;
  } else {
    configured_runtime_features_ &= ~feature;
  }
}

void TurboSessionPool::UpdateImageQuality() {
  for (ProxySessionList::iterator it = proxy_sessions_.begin();
       it != proxy_sessions_.end(); ++it)
    (*it)->SendImageQuality(configured_image_quality_);

  sent_image_quality_ = configured_image_quality_;
}

void TurboSessionPool::UpdateRuntimeFeatures() {
  for (ProxySessionList::iterator it = proxy_sessions_.begin();
       it != proxy_sessions_.end(); ++it)
    (*it)->SendUpdatedFeatures(configured_runtime_features_);

  sent_runtime_features_ = configured_runtime_features_;
}

void TurboSessionPool::UpdateServerSideLog() {
  for (ProxySessionList::iterator it = proxy_sessions_.begin();
       it != proxy_sessions_.end(); ++it)
    (*it)->SendServerSideLog(configured_server_side_log_);

  sent_server_side_log_ = configured_server_side_log_;
}

void TurboSessionPool::UpdateUsageStats() {
  for (ProxySessionList::iterator it = proxy_sessions_.begin();
       it != proxy_sessions_.end(); ++it)
    (*it)->SendUsageStats(kept_usage_stats_.Get(), kept_usage_stats_version_);

  sent_usage_stats_ = kept_usage_stats_.Get();
}

void TurboSessionPool::SendPageLoadTime(uintptr_t context, const GURL& url) {
  LoadInfoIterator iter = current_loads_.find(context);

  if (iter != current_loads_.end()) {
    if (!iter->second.HasStopTime())
      return;

    int64_t elapsed = iter->second.GetDuration().InMilliseconds();
    uint32_t elapsed32 = 0;
    if (elapsed > 0 && elapsed < std::numeric_limits<uint32_t>::max())
      elapsed32 = static_cast<uint32_t>(elapsed);

    if (elapsed32 && HasProxySession()) {
      GetProxySession()->SendPageLoadTime(IsTurboEnabled(), url, elapsed32);
    }

    current_loads_.erase(iter);
  }
}

std::string TurboSessionPool::GetDefaultServer() {
  DCHECK(!HttpStreamFactory::turbo1_enabled());
  // The server address is chosen as follows (in order of preference):
  // 1) command-line
  // 2) stored suggested server
  // 3) compile-time default
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  std::string server;
  if (command_line->HasSwitch("turbo-proxy")) {
    server = command_line->GetSwitchValueASCII("turbo-proxy");
  } else {
    server = session_->network_delegate()->NotifyTurboGetDefaultServer();
  }

  if (server.empty())
    server = TURBO2_PROXY;

  if (server.find(':') == std::string::npos) {
    server += ':';
    server +=
        base::IntToString(compile_time_turbo2_proxy_.host_port_pair().port());
  }

  return server;
}

void TurboSessionPool::InitializeTurbo2Proxies() {
  DCHECK(!HttpStreamFactory::turbo1_enabled());
  std::string proxy_uri = GetDefaultServer();
  turbo2_proxies_.clear();
  int num_sessions = kParallelSessionsCount;
  while (num_sessions--) {
    ProxyServer proxy = net::ProxyServer::FromURI(
        proxy_uri, compile_time_turbo2_proxy_.scheme());
    proxy.set_turbo_connection_id(proxy_connection_id_counter_++);
    turbo2_proxies_.push_back(proxy);
  }
}

void TurboSessionPool::ResetSuggestedServerCooldown() {
  suggested_server_cooldown_ = false;
}

TurboSessionPool::LoadInfo::LoadInfo(base::Time start_time)
    : start_time_(start_time) {
}

void TurboSessionPool::LoadInfo::SetStopTime(base::Time stop_time) {
  stop_time_ = stop_time;
}

bool TurboSessionPool::LoadInfo::HasStopTime() const {
  return !stop_time_.is_null();
}

base::TimeDelta TurboSessionPool::LoadInfo::GetDuration() const {
  if (stop_time_ > start_time_)
    return stop_time_ - start_time_;
  return base::TimeDelta();
}

}  // namespace net
