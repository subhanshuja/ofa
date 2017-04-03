// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/core_impl/syncapi_server_connection_manager.h"

#include <stdint.h>

#include "components/sync/core/http_post_provider_factory.h"
#include "components/sync/core/http_post_provider_interface.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"

#if defined(OPERA_SYNC)
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#endif  // OPERA_SYNC

namespace syncer {

SyncAPIBridgedConnection::SyncAPIBridgedConnection(
    ServerConnectionManager* scm,
    HttpPostProviderFactory* factory)
    : Connection(scm), factory_(factory) {
  post_provider_ = factory_->Create();
}

SyncAPIBridgedConnection::~SyncAPIBridgedConnection() {
  DCHECK(post_provider_);
  factory_->Destroy(post_provider_);
  post_provider_ = NULL;
}

bool SyncAPIBridgedConnection::Init(
    const char* path,
    const std::string& auth_token,
    const std::string& payload,
    HttpResponse* response
#if defined(OPERA_SYNC)
    ,
    const sync_oauth_signer::SignParams& sign_params
#endif
    ) {
  std::string sync_server;
  int sync_server_port = 0;
  bool use_ssl = false;
  GetServerParams(&sync_server, &sync_server_port, &use_ssl);
  std::string connection_url = MakeConnectionURL(sync_server, path, use_ssl);

  HttpPostProviderInterface* http = post_provider_;
  http->SetURL(connection_url.c_str(), sync_server_port);

#if defined(OPERA_SYNC)
  if (sign_params.IsValid()) {
    std::string headers;
    std::string auth_header = sign_params.GetSignedAuthHeader(connection_url);
    headers = "Authorization: " + auth_header;
    VLOG(1) << "OAuth header: " << auth_header;
    http->SetExtraRequestHeaders(headers.c_str());
  } else
#endif
  if (!auth_token.empty()) {
    std::string headers;
    headers = "Authorization: Bearer " + auth_token;
    http->SetExtraRequestHeaders(headers.c_str());
  }

  // Must be octet-stream, or the payload may be parsed for a cookie.
  http->SetPostPayload("application/octet-stream", payload.length(),
                       payload.data());

  // Issue the POST, blocking until it finishes.
  int error_code = 0;
  int response_code = 0;
  if (!http->MakeSynchronousPost(&error_code, &response_code)) {
    DCHECK_NE(error_code, net::OK);
#if defined(OPERA_SYNC)
    VLOG(1) << "Http POST failed, error returns: " << error_code;
#else
    DVLOG(1) << "Http POST failed, error returns: " << error_code;
#endif  // OPERA_SYNC
    response->server_status = HttpResponse::CONNECTION_UNAVAILABLE;
    return false;
  }

  // We got a server response, copy over response codes and content.
  response->response_code = response_code;
  response->content_length =
      static_cast<int64_t>(http->GetResponseContentLength());
  response->payload_length =
      static_cast<int64_t>(http->GetResponseContentLength());
#if defined(OPERA_SYNC)
  response->auth_problem.set_token(auth_token);
  response->auth_problem.set_source(opera_sync::OperaAuthProblem::SOURCE_SYNC);
#endif  // OPERA_SYNC

  if (response->response_code < 400) {
    response->server_status = HttpResponse::SERVER_CONNECTION_OK;
  } else if (response->response_code == net::HTTP_UNAUTHORIZED) {
#if defined(OPERA_SYNC)
    const std::string& reason =
        http->GetResponseHeaderValue("X-Opera-Auth-Reason");
    int reason_int;
    if (!base::StringToInt(reason, &reason_int)) {
      LOG(WARNING) << "Cannot convert reason code got from server: " << reason;
    }
    response->auth_problem.set_auth_errcode(reason_int);
    response->server_status = HttpResponse::SYNC_AUTH_ERROR;
  } else if (response->response_code >= 500 && response->response_code < 600) {
    // DNA-45152 - our infrastructure can't control the load balancer that can
    // return HTTP 5xx codes in unexpected conditions. Treat all of them as
    // a non-fatal transient error.
    response->server_status = HttpResponse::LOAD_BALANCER_ERROR;
    // We'd like to observe a value from range [0, 99] so that we don't grow
    // the bucket count without a real reason.
    int histogram_value = response->response_code - 500;
    UMA_HISTOGRAM_ENUMERATION("Opera.DSK.Sync.HTTP500Response",
        histogram_value, 100);
#endif  // OPERA_SYNC
  } else {
    response->server_status = HttpResponse::SYNC_SERVER_ERROR;
  }

  // Write the content into our buffer.
  buffer_.assign(http->GetResponseContent(), http->GetResponseContentLength());
  return true;
}

void SyncAPIBridgedConnection::Abort() {
  DCHECK(post_provider_);
  post_provider_->Abort();
}

SyncAPIServerConnectionManager::SyncAPIServerConnectionManager(
    const std::string& server,
    int port,
    bool use_ssl,
#if defined(OPERA_SYNC)
    const sync_oauth_signer::SignParams& sign_params,
#endif
    HttpPostProviderFactory* factory,
    CancelationSignal* cancelation_signal)
    : ServerConnectionManager(server, port, use_ssl, cancelation_signal
#if defined(OPERA_SYNC)
      , sign_params
#endif  // OPERA_SYNC
  ),
      post_provider_factory_(factory) {
  DCHECK(post_provider_factory_.get());
}

SyncAPIServerConnectionManager::~SyncAPIServerConnectionManager() {}

ServerConnectionManager::Connection*
SyncAPIServerConnectionManager::MakeConnection() {
  return new SyncAPIBridgedConnection(this, post_provider_factory_.get());
}

}  // namespace syncer
