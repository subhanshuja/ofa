// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_REQUEST_INFO_H__
#define NET_HTTP_HTTP_REQUEST_INFO_H__

#include <string>

#include "net/base/net_export.h"
#include "net/base/privacy_mode.h"
#include "net/cookies/cookie_store.h"
#include "net/http/http_request_headers.h"
#include "url/gurl.h"

namespace net {

class UploadDataStream;

struct NET_EXPORT HttpRequestInfo {
  enum RequestMotivation{
    // TODO(mbelshe): move these into Client Socket.
    PRECONNECT_MOTIVATED,  // Request was motivated by a prefetch.
    OMNIBOX_MOTIVATED,     // Request was motivated by the omnibox.
    NORMAL_MOTIVATION,     // No special motivation associated with the request.
    EARLY_LOAD_MOTIVATED,  // When browser asks a tab to open an URL, this short
                           // circuits that path (of waiting for the renderer to
                           // do the URL request), and starts loading ASAP.
  };

  HttpRequestInfo();
  HttpRequestInfo(const HttpRequestInfo& other);
  ~HttpRequestInfo();

  // The requested URL.
  GURL url;

  // The method to use (GET, POST, etc.).
  std::string method;

  // Any extra request headers (including User-Agent).
  HttpRequestHeaders extra_headers;

  // Any upload data.
  UploadDataStream* upload_data_stream;

  // Any load flags (see load_flags.h).
  int load_flags;

  // The motivation behind this request.
  RequestMotivation motivation;

  // An optional globally unique identifier for this request for use by the
  // consumer. 0 is invalid.
  uint64_t request_id;

  // If enabled, then request must be sent over connection that cannot be
  // tracked by the server (e.g. without channel id).
  PrivacyMode privacy_mode;

  // Used for the X-Opera-RequestType header, if the request is sent through the
  // Turbo server.
  std::string turbo_request_type;

  // Used for the X-Opera-Main header, if the request is sent through the Turbo
  // server.
  std::string turbo_main;

  // Used to synchronize cookies to the Turbo server if/when needed.
  CookieStore* cookie_store;

  // If present, the host of the referrer whose TokenBindingID should be
  // included in a referred TokenBinding.
  std::string token_binding_referrer;
};

}  // namespace net

#endif  // NET_HTTP_HTTP_REQUEST_INFO_H__
