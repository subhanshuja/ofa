// Copyright (c) 2015 Opera Software ASA. All rights reserved.

#ifndef NET_TURBO_TURBO_SESSION_POOL_H_
#define NET_TURBO_TURBO_SESSION_POOL_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/lazy_instance.h"
#include "base/memory/weak_ptr.h"
#include "net/proxy/proxy_server.h"
#include "net/spdy/spdy_session.h"
#include "net/spdy/spdy_session_pool.h"
#include "net/turbo/turbo_constants.h"

namespace net {

class HttpNetworkSession;
class HttpStreamRequest;
class TurboStreamRequestDelegate;
class TurboCacheSynchronizer;
class TurboCookieSynchronizer;

// This class tracks the lifetime of all Turbo connections. It also routes
// HTTP requests to the appropriate Turbo connections.
class NET_EXPORT TurboSessionPool : public SpdySessionPool::Observer,
                                    public SpdySession::Observer {
 public:
  explicit TurboSessionPool(HttpNetworkSession* session);
  ~TurboSessionPool() override;

  HttpNetworkSession* http_network_session() { return session_; }
  TurboCookieSynchronizer* turbo_cookie_synchronizer() {
    return turbo_cookie_synchronizer_.get();
  }
  TurboCacheSynchronizer* turbo_cache_synchronizer() {
    return turbo_cache_synchronizer_.get();
  }

  // TODO(michalp): What exactly is the purpose of this method? Figure out a
  // more descriptive name.
  void CheckTurboEnabled();
  void ReportUnclaimedPushedStreams();

  bool HasProxySession() { return !proxy_sessions_.empty(); }
  SpdySession* GetProxySession() {
    DCHECK(HasProxySession());
    return *proxy_sessions_.begin();
  }

  ProxyServer GetProxyServerForRequest(const HttpRequestInfo& request_info);

  void SendUnclaimed();

  void SignalLoadStart(uintptr_t context, const GURL& url);
  void SignalProvisionalLoadStart(uintptr_t context, const GURL& url);
  void SignalLoadRedirect(uintptr_t context,
                          const GURL& source_url,
                          const GURL& target_url);
  void SignalLoadStop(uintptr_t context, const GURL& url);

  void SignalConnectionFailed();

  static void SetImageQuality(TurboImageQuality quality);
  static int image_quality() { return configured_image_quality_; }

  static void SetVideoCompressionEnabled(bool enable);
  static bool video_compression_enabled() {
    return (configured_runtime_features_ &
            TURBO_FEATURE_VIDEO_COMPRESSION) != 0;
  }

  static void SetAdblockBlankEnabled(bool enable);
  static bool adblock_blank_enabled() {
    return (configured_runtime_features_ & TURBO_FEATURE_ADBLOCK_BLANK) != 0;
  }

  static void SetCompresSsl(bool enable);
  static bool compress_ssl() {
    return (configured_runtime_features_ & TURBO_FEATURE_HTTPS_REQUESTS) != 0;
  }

  static uint32_t runtime_features() { return configured_runtime_features_; }

  static void SetServerSideLog(bool enable);

  static void SetUsageStats(const std::string& stats, uint8_t stats_version);

  base::WeakPtr<TurboSessionPool> AsWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  friend class TurboStreamRequestDelegate;

  // SpdySessionPool::Observer implementation
  void OnTurboSpdySessionCreated(
      const base::WeakPtr<SpdySession> session) override;
  void OnTurboSpdySessionInitialized(
      const base::WeakPtr<SpdySession> session) override;
  void OnTurboSpdySessionDestroyed(
      const base::WeakPtr<SpdySession> session) override;

  // SpdySession::Observer implementation
  void OnSynStream(const GURL& gurl) override;
  void OnSynReply(const GURL& gurl) override;
  void OnTurboSession(bool is_secure,
                      std::string* client_id,
                      std::string* device_id,
                      std::string* mcc,
                      std::string* mnc,
                      std::string* key,
                      std::map<std::string, std::string>* headers) override;
  void OnTurboStatistics(uint32_t compressed, uint32_t uncompressed) override;
  void OnTurboSuggestedServer(const std::string& suggested_server) override;

  void ConnectToProxy();
  void CleanUpStreamRequest();

  static void SetRuntimeFeature(uint32_t feature, bool enable);

  void UpdateImageQuality();
  void UpdateRuntimeFeatures();
  void UpdateServerSideLog();
  void UpdateUsageStats();
  void SendPageLoadTime(uintptr_t context, const GURL& url);
  std::string GetDefaultServer();
  void InitializeTurbo2Proxies();
  void ResetSuggestedServerCooldown();

  HttpNetworkSession* session_;

  class LoadInfo {
   public:
    LoadInfo() {}
    explicit LoadInfo(base::Time start_time);

    base::TimeDelta GetDuration() const;
    base::Time StartTime() const { return start_time_; }
    bool HasStopTime() const;
    void SetStopTime(base::Time stop_time);

   private:
    base::Time start_time_, stop_time_;
  };

  bool was_turbo_enabled_;

  // Mapping of context to some information for current ongoing loads.
  std::map<uintptr_t, LoadInfo> current_loads_;
  typedef std::map<uintptr_t, LoadInfo>::iterator LoadInfoIterator;

  // Current Turbo proxy sessions.
  typedef std::vector<SpdySession*> ProxySessionList;
  ProxySessionList proxy_sessions_;
  ProxySessionList::size_type proxy_session_selector_;

  ProxyServer turbo1_proxy_;
  // We store this to retain the scheme and port specified in the compile-time
  // default. These are used as a fallback e.g. for suggested server, which
  // currently only specifies the domain name.
  ProxyServer compile_time_turbo2_proxy_;
  std::vector<ProxyServer> turbo2_proxies_;

  // Used when explicitly creating Turbo 2 connections from this class.
  std::unique_ptr<TurboStreamRequestDelegate> stream_request_delegate_;
  std::vector<HttpStreamRequest*> stream_requests_;

  std::unique_ptr<TurboCacheSynchronizer> turbo_cache_synchronizer_;
  std::unique_ptr<TurboCookieSynchronizer> turbo_cookie_synchronizer_;

  // Used to select a connection to the default Turbo 2 proxy in a
  // round-robin way if no other routing rule applies.
  int proxy_connection_id_counter_;

  // Is the cooldown from last received SUGGESTED_SERVER active?
  bool suggested_server_cooldown_;

  int sent_image_quality_;
  static int configured_image_quality_;
  uint32_t sent_runtime_features_;
  static uint32_t configured_runtime_features_;
  bool sent_server_side_log_;
  static bool configured_server_side_log_;
  std::string sent_usage_stats_;
  static base::LazyInstance<std::string> kept_usage_stats_;
  static uint8_t kept_usage_stats_version_;
  base::WeakPtrFactory<TurboSessionPool> weak_ptr_factory_;
};

}  // namespace net

#endif  // NET_TURBO_TURBO_SESSION_POOL_H_
