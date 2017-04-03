// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef JINGLE_NOTIFIER_BASE_NOTIFIER_OPTIONS_H_
#define JINGLE_NOTIFIER_BASE_NOTIFIER_OPTIONS_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "jingle/notifier/base/notification_method.h"
#include "net/base/host_port_pair.h"
#include "net/url_request/url_request_context_getter.h"

#if defined(OPERA_SYNC)
#include "jingle/notifier/base/token_generator.h"
#endif  // OPERA_SYNC

namespace notifier {

struct NotifierOptions {
  NotifierOptions();
  NotifierOptions(const NotifierOptions& other);
  ~NotifierOptions();

  // Indicates that the SSLTCP port (443) is to be tried before the the XMPP
  // port (5222) during login.
  bool try_ssltcp_first;

  // Indicates that insecure connections (e.g., plain authentication,
  // no TLS) are allowed.  Only used for testing.
  bool allow_insecure_connection;

  // Indicates that the login info passed to XMPP is invalidated so
  // that login fails.
  bool invalidate_xmpp_login;

  // Contains a custom URL and port for the notification server, if one is to
  // be used. Empty otherwise.
  net::HostPortPair xmpp_host_port;

#if defined(OPERA_SYNC)
  uint16_t fallback_port;
#endif  // OPERA_SYNC

  // Indicates the method used by sync clients while sending and listening to
  // notifications.
  NotificationMethod notification_method;

  // Specifies the auth mechanism to use ("X-GOOGLE-TOKEN", "X-OAUTH2", etc),
  std::string auth_mechanism;

  // The URLRequestContextGetter to use for doing I/O.
  scoped_refptr<net::URLRequestContextGetter> request_context_getter;

#if defined(OPERA_SYNC)
  // The notifier needs to create new tokens for every attempted connection.
  // A token contains an oauth_nonce and oauth_timestamp which need to be
  // unique. This callback generates a fresh, signed token.
  // Assume this will be called on threads other than UI.
  opera::GenerateTokenCallback token_generator;
#endif  // OPERA_SYNC
};

}  // namespace notifier

#endif  // JINGLE_NOTIFIER_BASE_NOTIFIER_OPTIONS_H_
