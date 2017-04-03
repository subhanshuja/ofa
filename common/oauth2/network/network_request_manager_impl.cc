// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/network/network_request_manager_impl.h"

#include <string>

#include "base/message_loop/message_loop.h"
#include "base/memory/ptr_util.h"
#include "base/threading/thread_checker_impl.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_fetcher.h"
#include "components/sync/api/time.h"

#include "common/net/sensitive_url_request_user_data.h"
#include "common/oauth2/diagnostics/diagnostic_service.h"
#include "common/oauth2/network/network_request.h"
#include "common/oauth2/util/constants.h"

namespace opera {
namespace oauth2 {

NetworkRequestManagerImpl::OngoingRequest::LastResponse::LastResponse()
    : http_code(0),
      response_status(RS_UNSET),
      response_time(base::Time()),
      url_request_status(net::URLRequestStatus()) {}

NetworkRequestManagerImpl::OngoingRequest::OngoingRequest()
    : last_request_time(base::Time()),
      next_request_time(base::Time()),
      request_count(0),
      fetcher(nullptr) {}

NetworkRequestManagerImpl::OngoingRequest::~OngoingRequest() {}

NetworkRequestManagerImpl::NetworkRequestManagerImpl(
    DiagnosticService* diagnostic_service,
    RequestUrlsConfig base_request_urls,
    net::URLRequestContextGetter* url_request_context_getter,
    net::BackoffEntry::Policy backoff_policy,
    std::unique_ptr<base::TickClock> backoff_clock,
    std::unique_ptr<base::Clock> clock,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : diagnostic_service_(diagnostic_service),
      url_request_context_getter_(url_request_context_getter),
      backoff_policy_(backoff_policy),
      backoff_clock_(std::move(backoff_clock)),
      clock_(std::move(clock)),
      task_runner_(task_runner),
      weak_ptr_factory_(this) {
  VLOG(4) << "Request manager created.";
  base_request_urls_ = base_request_urls;

  DCHECK(url_request_context_getter_);
  DCHECK(clock_);

  if (diagnostic_service_) {
    diagnostic_service_->AddSupplier(this);
  }
}

NetworkRequestManagerImpl::~NetworkRequestManagerImpl() {
  DCHECK(thread_checker_.CalledOnValidThread());
  VLOG(4) << "Request manager destroyed.";
  if (diagnostic_service_) {
    diagnostic_service_->RemoveSupplier(this);
  }
}

void NetworkRequestManagerImpl::OnURLFetchComplete(
    const net::URLFetcher* source) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(source);
  OngoingRequest* request = FindRequestByFetcher(source);
  if (!request) {
    VLOG(1) << "Request has been cancelled along the way.";
    return;
  }

  scoped_refptr<NetworkRequest> network_request = request->network_request;
  DCHECK(network_request.get());
  request->last_response.reset(new OngoingRequest::LastResponse);
  request->last_response->response_time = clock_->Now();
  request->last_response->url_request_status = source->GetStatus();
  int http_code = source->GetResponseCode();
  request->last_response->http_code = http_code;

  VLOG(4) << "Request to " << source->GetOriginalURL().spec()
          << " finished with status "
          << net::ErrorToString(source->GetStatus().ToNetError())
          << ", HTTP status " << http_code << " and URL "
          << source->GetURL().spec() << ".";

  // In case the remote service responds with a redirect, we expect a CANCELLED
  // status along with the response HTTP status. This behaviour is defined by
  // URLFetcher::SetStopOnRedirect() documentation.
  // In such a case we don't want to follow the redirect, but we have to do
  // something, the best option appears to be to pass the response to
  // TryResponse so that a particular response handler has a chance to process
  // it, such a 3xx response may be meaningful for any request endpoint. In case
  // it's not we'll just get a HTTP_PROBLEM like with any other not expected
  // HTTP status.
  const auto fetcher_status = source->GetStatus().status();
  bool is_success = (fetcher_status == net::URLRequestStatus::SUCCESS);
  bool http_code_is_3xx = ((http_code >= 300) && (http_code <= 399));
  bool is_redirect = ((fetcher_status == net::URLRequestStatus::CANCELED) &&
                      (http_code_is_3xx));

  if (is_success || is_redirect) {
    std::string response_data;
    bool ok = source->GetResponseAsString(&response_data);
    DCHECK(ok);
    VLOG(1) << "Response content is "
            << (response_data.empty() ? "empty" : response_data);

    NetworkResponseStatus status = RS_UNSET;
    int64_t throttle_delay_seconds = -1;
    if (source->GetResponseHeaders()) {
      throttle_delay_seconds =
          source->GetResponseHeaders()->GetInt64HeaderValue(kRetryAfter);
    }
    // 429 Too Many Requests
    if (http_code == net::HTTP_TOO_MANY_REQUESTS) {
      if (throttle_delay_seconds != -1) {
        VLOG(4) << "Got Retry-After: " << throttle_delay_seconds;
        status = RS_THROTTLED;
      } else {
        // Fall back to default backoff delay in case the server did omit the
        // Retry-After HTTP header.
        status = RS_HTTP_PROBLEM;
      }
    } else {
      status = network_request->TryResponse(http_code, response_data);
    }

    request->last_response->response_status = status;
    RequestTakeSnapshot();

    if (status == RS_OK) {
      VLOG(2) << "Response is valid.";
      if (request->consumer.get()) {
        request->consumer->OnNetworkRequestFinished(network_request, status);
      } else {
        VLOG(1) << "Consumer gone already.";
      }
      // The consumer may cancel all requests while in
      // OnNetworkRequestFinished(). Check again.
      if (FindRequestByFetcher(source)) {
        EraseRequest(request);
      }
      request = nullptr;
      RequestTakeSnapshot();
    } else if (status == RS_HTTP_PROBLEM) {
      VLOG(1) << "Response HTTP code is not valid. Rescheduling request.";
      RescheduleRequest(request, base::TimeDelta());
    } else if (status == RS_PARSE_PROBLEM) {
      VLOG(1) << "Response cannot be parsed. Rescheduling request.";
      RescheduleRequest(request, base::TimeDelta());
    } else if (status == RS_THROTTLED) {
      base::TimeDelta throttle_delay =
          base::TimeDelta::FromSeconds(throttle_delay_seconds);
      DCHECK_NE(base::TimeDelta(), throttle_delay);
      VLOG(1) << "Server requested throttling for " << throttle_delay;
      RescheduleRequest(request, throttle_delay);
    } else {
      NOTREACHED();
    }
  } else {
    VLOG(1) << "Response status is unexpected. Rescheduling request.";
    RescheduleRequest(request, base::TimeDelta());
  }
  if (request) {
    request->fetcher = nullptr;
  }
}

void NetworkRequestManagerImpl::StartRequest(
    scoped_refptr<NetworkRequest> request,
    base::WeakPtr<Consumer> consumer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(request);
  DCHECK(consumer.get());
  StartDelayedRequest(request, consumer, base::TimeDelta());
}

void NetworkRequestManagerImpl::StartDelayedRequest(
    scoped_refptr<NetworkRequest> network_request,
    base::WeakPtr<Consumer> consumer,
    base::TimeDelta delay) {
  DCHECK(thread_checker_.CalledOnValidThread());
  OngoingRequest* request = FindRequestByNetworkRequest(network_request);
  if (!request) {
    request = new OngoingRequest;
    request->backoff.reset(
        new net::BackoffEntry(&backoff_policy_, backoff_clock_.get()));
    request->network_request = network_request;
    request->consumer = consumer;
    requests_.push_back(base::WrapUnique(request));
  }
  DCHECK_EQ(consumer.get(), request->consumer.get());
  request->next_request_time = clock_->Now() + delay;
  RequestTakeSnapshot();
  task_runner_->PostDelayedTask(
      FROM_HERE, base::Bind(&NetworkRequestManagerImpl::DoStartRequest,
                            weak_ptr_factory_.GetWeakPtr(), network_request),
      delay);
}

std::string NetworkRequestManagerImpl::GetDiagnosticName() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return "network_manager";
}

void NetworkRequestManagerImpl::CancelAllRequests() {
  DCHECK(thread_checker_.CalledOnValidThread());
  requests_.clear();
}

std::unique_ptr<base::DictionaryValue>
NetworkRequestManagerImpl::GetDiagnosticSnapshot() {
  DCHECK(thread_checker_.CalledOnValidThread());
  std::unique_ptr<base::DictionaryValue> snapshot(new base::DictionaryValue);
  std::unique_ptr<base::DictionaryValue> base_request_urls(
      new base::DictionaryValue);
  for (const auto& base_request_url_info : base_request_urls_) {
    RequestManagerUrlType type = base_request_url_info.first;
    GURL url = base_request_url_info.second.first;
    bool allow_insecure = base_request_url_info.second.second;
    std::string secure_only;
    if (allow_insecure) {
      secure_only = " [insecure allowed]";
    }
    base_request_urls->SetString(
        std::string(RequestManagerUrlTypeToString(type)),
        url.spec() + secure_only);
  }
  snapshot->Set("base_request_urls", std::move(base_request_urls));
  std::unique_ptr<base::ListValue> requests(new base::ListValue);
  for (const auto& request : requests_) {
    DCHECK(request.get());
    std::unique_ptr<base::DictionaryValue> detail(new base::DictionaryValue);
    const auto& network_request = request->network_request;
    DCHECK(network_request);
    std::string consumer_name = "** GONE **";
    if (request->consumer) {
      consumer_name = request->consumer->GetConsumerName();
    }
    detail->SetString("consumer", consumer_name);
    detail->SetString("base_request_url",
                      RequestManagerUrlTypeToString(
                          network_request->GetRequestManagerUrlType()));
    detail->SetString("path", network_request->GetPath());
    detail->SetInteger("type", network_request->GetHTTPRequestType());
    detail->SetString("detail", network_request->GetDiagnosticDescription());
    const auto last_response = request->last_response.get();
    if (last_response) {
      std::unique_ptr<base::DictionaryValue> last_response_desc(
          new base::DictionaryValue);
      last_response_desc->SetInteger("http_status", last_response->http_code);
      last_response_desc->SetString(
          "request_status",
          NetworkResponseStatusToString(last_response->response_status));
      last_response_desc->SetDouble("response_time",
                                    last_response->response_time.ToJsTime());
      last_response_desc->SetString(
          "network_error",
          net::ErrorToString(last_response->url_request_status.ToNetError()));
      detail->Set("last_response", std::move(last_response_desc));
    }

    detail->SetDouble("last_request_time",
                      request->last_request_time.ToJsTime());
    detail->SetDouble("next_request_time",
                      request->next_request_time.ToJsTime());
    detail->SetInteger("request_count", request->request_count);
    requests->Append(std::move(detail));
  }
  snapshot->Set("requests", std::move(requests));

  return snapshot;
}

// static
net::BackoffEntry::Policy NetworkRequestManagerImpl::GetDefaultBackoffPolicy() {
  return {
      // Number of initial errors (in sequence) to ignore before applying
      // exponential back-off rules.
      0,

      // Initial delay for exponential back-off in ms.
      1000,  // 1 second.

      // Factor by which the waiting time will be multiplied.
      2,

      // Fuzzing percentage. ex: 10% will spread requests randomly
      // between 90%-100% of the calculated time.
      0.33,  // 33%.

      // Maximum amount of time we are willing to delay our request in ms.
      5 * 60 * 1000,  // 5 minutes.

      // Time to keep an entry from being discarded even when it
      // has no significant state, -1 to never discard.
      -1,

      // Don't use initial delay unless the last request was an error.
      false,
  };
}

void NetworkRequestManagerImpl::DoStartRequest(
    scoped_refptr<NetworkRequest> network_request) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(network_request);
  OngoingRequest* request = FindRequestByNetworkRequest(network_request);
  if (!request) {
    VLOG(1) << "Request has been canceled along the way.";
    return;
  }

  DCHECK(!network_request->GetPath().empty());
  DCHECK_NE(RMUT_UNSET, network_request->GetRequestManagerUrlType());
  DCHECK_EQ(1u, base_request_urls_.count(
                    network_request->GetRequestManagerUrlType()));
  const auto& base_request_url_info =
      base_request_urls_.at(network_request->GetRequestManagerUrlType());

  GURL base_request_url = base_request_url_info.first;
  bool allow_insecure = base_request_url_info.second;

  if (!base_request_url.SchemeIs(url::kHttpsScheme) && !allow_insecure) {
    VLOG(1) << "Insecure connection to " << base_request_url.spec()
            << " blocked.";
    request->last_response.reset(new OngoingRequest::LastResponse);
    request->last_response->response_status = RS_INSECURE_CONNECTION_FORBIDDEN;
    RequestTakeSnapshot();
    if (request->consumer.get()) {
      request->consumer->OnNetworkRequestFinished(
          network_request, RS_INSECURE_CONNECTION_FORBIDDEN);
    }
    if (FindRequestByNetworkRequest(network_request)) {
      EraseRequest(request);
    }
    network_request = nullptr;
    request = nullptr;
    RequestTakeSnapshot();
    return;
  }

  GURL request_url = base_request_url.Resolve(network_request->GetPath());
  const auto query_string = network_request->GetQueryString();
  if (!query_string.empty()) {
    request_url = request_url.Resolve(std::string("?") + query_string);
  }
  VLOG(1) << "Starting request to " << request_url;
  VLOG(1) << "Request content is " << (network_request->GetContent().empty()
                                           ? "empty"
                                           : network_request->GetContent());

  std::unique_ptr<net::URLFetcher> fetcher = net::URLFetcher::Create(
      request_url, network_request->GetHTTPRequestType(), this);

  fetcher->SetAutomaticallyRetryOn5xx(false);
  fetcher->SetAutomaticallyRetryOnNetworkChanges(false);
  fetcher->SetUploadData(network_request->GetRequestContentType(),
                         request->network_request->GetContent());
  fetcher->SetRequestContext(url_request_context_getter_);
  fetcher->SetLoadFlags(network_request->GetLoadFlags());
  fetcher->SetURLRequestUserData(
      opera::SensitiveURLRequestUserData::kUserDataKey,
      base::Bind(&opera::SensitiveURLRequestUserData::Create));
  fetcher->SetStopOnRedirect(true);

  auto extra_headers = network_request->GetExtraRequestHeaders();
  for (const auto& extra_header : extra_headers) {
    VLOG(1) << "Extra request header: " << extra_header;
    fetcher->AddExtraRequestHeader(extra_header);
  }
  fetcher->Start();

  request->fetcher = fetcher.release();

  request->last_request_time = clock_->Now();
  request->next_request_time = base::Time();
  request->request_count++;

  RequestTakeSnapshot();
}

void NetworkRequestManagerImpl::RescheduleRequest(OngoingRequest* request,
                                                  base::TimeDelta delay) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(request);
  DCHECK(request->backoff);
  if (delay == base::TimeDelta()) {
    request->backoff->InformOfRequest(false);
    delay = request->backoff->GetTimeUntilRelease();
  }
  VLOG(1) << "Request scheduled with a delay " << delay;
  StartDelayedRequest(request->network_request, request->consumer, delay);
  return;
}

void NetworkRequestManagerImpl::RequestTakeSnapshot() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (diagnostic_service_) {
    diagnostic_service_->TakeSnapshot();
  }
}

NetworkRequestManagerImpl::OngoingRequest*
NetworkRequestManagerImpl::FindRequestByFetcher(
    const net::URLFetcher* fetcher) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(fetcher);
  for (const auto& r : requests_) {
    DCHECK(r.get());
    if (r->fetcher == fetcher) {
      return r.get();
    }
  }
  return nullptr;
}

NetworkRequestManagerImpl::OngoingRequest*
NetworkRequestManagerImpl::FindRequestByNetworkRequest(
    const scoped_refptr<NetworkRequest> request) const {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(request);
  for (const auto& r : requests_) {
    DCHECK(r.get());
    if (r->network_request == request) {
      return r.get();
    }
  }
  return nullptr;
}

void NetworkRequestManagerImpl::EraseRequest(OngoingRequest* request) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(request);
  for (auto it = requests_.begin(); it != requests_.end(); it++) {
    if (it->get() == request) {
      requests_.erase(it);
      return;
    }
  }
  NOTREACHED();
}

}  // namespace oauth2
}  // namespace opera
