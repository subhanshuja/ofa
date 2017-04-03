// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_stream_factory.h"

#include "base/logging.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "net/base/host_mapping_rules.h"
#include "net/base/host_port_pair.h"
#include "net/base/parse_number.h"
#include "net/base/port_util.h"
#include "net/http/http_network_session.h"
#include "net/http/http_response_headers.h"
#include "net/quic/core/quic_protocol.h"
#include "net/spdy/spdy_alt_svc_wire_format.h"
#include "url/gurl.h"

namespace net {

// static
bool HttpStreamFactory::turbo1_enabled_ = false;
// static
bool HttpStreamFactory::turbo2_enabled_ = false;
// static
bool HttpStreamFactory::have_turbo_server_features_ = false;
// static
uint32_t HttpStreamFactory::turbo_server_features_ = 0;
// static
std::string* HttpStreamFactory::turbo_client_id_ = NULL;
// static
bool HttpStreamFactory::proxy_for_local_servers_enabled_ = false;

HttpStreamFactory::~HttpStreamFactory() {}

// static
void HttpStreamFactory::ResetStaticSettingsToInit() {
  if (turbo_client_id_)
    delete turbo_client_id_;
  turbo1_enabled_ = false;
  turbo2_enabled_ = false;
  have_turbo_server_features_ = false;
  turbo_server_features_ = 0;
  proxy_for_local_servers_enabled_ = false;
  turbo_client_id_ = NULL;
}

// static
void HttpStreamFactory::set_turbo1_enabled(bool value) {
  turbo1_enabled_ = value;
#if defined(TURBO2_CONNECT_ON_STARTUP)
  LOG(ERROR) << "TURBO2_CONNECT_ON_STARTUP defined but Turbo 1 enabled.";
#endif
}

// static
void HttpStreamFactory::set_turbo2_enabled(bool value) {
#if defined(TURBO2_PROXY_AUTH_SECRET)
  turbo2_enabled_ = value;
#else
  LOG(ERROR) << "TURBO2_PROXY_AUTH_SECRET not defined; can't enable Turbo 2.";
#endif
}

// static
void HttpStreamFactory::SetTurboClientId(std::string value) {
  turbo_client_id_ = new std::string(value);
}

// static
std::string HttpStreamFactory::GetTurboClientId() {
  if (!turbo_client_id_) {
    const size_t kTurboClientIDSize = 32;
    std::string randomness = base::RandBytesAsString(kTurboClientIDSize);

    SetTurboClientId(randomness);
  }

  return *turbo_client_id_;
}

void HttpStreamFactory::ProcessAlternativeServices(
    HttpNetworkSession* session,
    const HttpResponseHeaders* headers,
    const url::SchemeHostPort& http_server) {
  if (!headers->HasHeader(kAlternativeServiceHeader))
    return;

  std::string alternative_service_str;
  headers->GetNormalizedHeader(kAlternativeServiceHeader,
                               &alternative_service_str);
  SpdyAltSvcWireFormat::AlternativeServiceVector alternative_service_vector;
  if (!SpdyAltSvcWireFormat::ParseHeaderFieldValue(
          alternative_service_str, &alternative_service_vector)) {
    return;
  }

  // Convert SpdyAltSvcWireFormat::AlternativeService entries
  // to net::AlternativeServiceInfo.
  AlternativeServiceInfoVector alternative_service_info_vector;
  for (const SpdyAltSvcWireFormat::AlternativeService&
           alternative_service_entry : alternative_service_vector) {
    AlternateProtocol protocol =
        AlternateProtocolFromString(alternative_service_entry.protocol_id);
    if (!IsAlternateProtocolValid(protocol) ||
        !session->IsProtocolEnabled(protocol) ||
        !IsPortValid(alternative_service_entry.port)) {
      continue;
    }
    // Check if QUIC version is supported.
    if (protocol == QUIC && !alternative_service_entry.version.empty()) {
      bool match_found = false;
      for (QuicVersion supported : session->params().quic_supported_versions) {
        for (uint16_t advertised : alternative_service_entry.version) {
          if (supported == advertised) {
            match_found = true;
            break;
          }
        }
        if (match_found) {
          break;
        }
      }
      if (!match_found) {
        continue;
      }
    }
    AlternativeService alternative_service(protocol,
                                           alternative_service_entry.host,
                                           alternative_service_entry.port);
    base::Time expiration =
        base::Time::Now() +
        base::TimeDelta::FromSeconds(alternative_service_entry.max_age);
    AlternativeServiceInfo alternative_service_info(alternative_service,
                                                    expiration);
    alternative_service_info_vector.push_back(alternative_service_info);
  }

  session->http_server_properties()->SetAlternativeServices(
      RewriteHost(http_server), alternative_service_info_vector);
}

HttpStreamFactory::HttpStreamFactory() {}

url::SchemeHostPort HttpStreamFactory::RewriteHost(
    const url::SchemeHostPort& server) {
  HostPortPair host_port_pair(server.host(), server.port());
  const HostMappingRules* mapping_rules = GetHostMappingRules();
  if (mapping_rules)
    mapping_rules->RewriteHost(&host_port_pair);
  return url::SchemeHostPort(server.scheme(), host_port_pair.host(),
                             host_port_pair.port());
}

}  // namespace net
