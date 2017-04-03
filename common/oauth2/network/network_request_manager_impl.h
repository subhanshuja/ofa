// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_NETWORK_NETWORK_REQUEST_MANAGER_IMPL_H_
#define COMMON_OAUTH2_NETWORK_NETWORK_REQUEST_MANAGER_IMPL_H_

#include <map>
#include <utility>

#include "base/time/clock.h"
#include "base/memory/weak_ptr.h"
#include "net/base/backoff_entry.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

#include "common/oauth2/diagnostics/diagnostic_supplier.h"
#include "common/oauth2/network/network_request_manager.h"
#include "common/oauth2/util/init_params.h"
#include "common/oauth2/util/util.h"

namespace net {
class URLRequestContextGetter;
}

namespace opera {
namespace oauth2 {

class DiagnosticService;

class NetworkRequestManagerImpl : public NetworkRequestManager,
                                  public DiagnosticSupplier,
                                  public net::URLFetcherDelegate {
 public:
  typedef std::map<RequestManagerUrlType, std::pair<GURL, bool>>
      RequestUrlsConfig;
  NetworkRequestManagerImpl(
      DiagnosticService* diagnostic_service,
      RequestUrlsConfig base_request_urls,
      net::URLRequestContextGetter* url_request_context_getter,
      net::BackoffEntry::Policy backoff_policy,
      std::unique_ptr<base::TickClock> backoff_clock,
      std::unique_ptr<base::Clock> clock,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);
  ~NetworkRequestManagerImpl() override;

  // net::URLFetcherDelegate implementation
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  void StartRequest(scoped_refptr<NetworkRequest> request,
                    base::WeakPtr<Consumer> consumer) override;

  // DiagnosticSupplier override.
  std::string GetDiagnosticName() const override;

  void CancelAllRequests() override;

  std::unique_ptr<base::DictionaryValue> GetDiagnosticSnapshot() override;

  static net::BackoffEntry::Policy GetDefaultBackoffPolicy();

 private:
  struct OngoingRequest {
    struct LastResponse {
      LastResponse();

      int http_code;
      NetworkResponseStatus response_status;
      base::Time response_time;
      net::URLRequestStatus url_request_status;
    };
    OngoingRequest();
    ~OngoingRequest();

    std::unique_ptr<net::BackoffEntry> backoff;
    base::Time last_request_time;
    base::Time next_request_time;
    int request_count;
    net::URLFetcher* fetcher;
    scoped_refptr<NetworkRequest> network_request;
    base::WeakPtr<Consumer> consumer;
    std::unique_ptr<LastResponse> last_response;
  };

  void StartDelayedRequest(scoped_refptr<NetworkRequest> request,
                           base::WeakPtr<Consumer> consumer,
                           base::TimeDelta delay);

  void DoStartRequest(scoped_refptr<NetworkRequest> request);
  void RescheduleRequest(OngoingRequest* request, base::TimeDelta delay);

  void RequestTakeSnapshot();

  OngoingRequest* FindRequestByFetcher(const net::URLFetcher* fetcher) const;
  OngoingRequest* FindRequestByNetworkRequest(
      const scoped_refptr<NetworkRequest> request) const;
  void EraseRequest(OngoingRequest* request);

  RequestUrlsConfig base_request_urls_;
  DiagnosticService* diagnostic_service_;
  net::URLRequestContextGetter* url_request_context_getter_;
  const net::BackoffEntry::Policy backoff_policy_;
  std::unique_ptr<base::TickClock> backoff_clock_;
  std::unique_ptr<base::Clock> clock_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  std::vector<std::unique_ptr<OngoingRequest>> requests_;

  base::ThreadCheckerImpl thread_checker_;

  base::WeakPtrFactory<NetworkRequestManagerImpl> weak_ptr_factory_;
};
}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_NETWORK_NETWORK_REQUEST_MANAGER_IMPL_H_
