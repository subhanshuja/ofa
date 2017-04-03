// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "jingle/notifier/communicator/login.h"

#include <string>

#include "base/logging.h"
#include "base/rand_util.h"
#include "base/time/time.h"
#include "net/base/host_port_pair.h"
#include "third_party/webrtc/libjingle/xmllite/xmlelement.h"
#include "webrtc/base/common.h"
#include "webrtc/base/firewallsocketserver.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/physicalsocketserver.h"
#include "webrtc/base/taskrunner.h"
#include "webrtc/libjingle/xmpp/asyncsocket.h"
#include "webrtc/libjingle/xmpp/prexmppauth.h"
#include "webrtc/libjingle/xmpp/xmppclient.h"
#include "webrtc/libjingle/xmpp/xmppclientsettings.h"
#include "webrtc/libjingle/xmpp/xmppengine.h"

namespace notifier {

Login::Delegate::~Delegate() {}

Login::Login(Delegate* delegate,
             const buzz::XmppClientSettings& user_settings,
             const scoped_refptr<net::URLRequestContextGetter>&
                request_context_getter,
             const ServerList& servers,
             bool try_ssltcp_first,
             const std::string& auth_mechanism)
    : delegate_(delegate),
      login_settings_(user_settings,
                      request_context_getter,
                      servers,
                      try_ssltcp_first,
                      auth_mechanism) {
  net::NetworkChangeNotifier::AddIPAddressObserver(this);
  net::NetworkChangeNotifier::AddConnectionTypeObserver(this);
  // TODO(akalin): Add as DNSObserver once bug 130610 is fixed.
  ResetReconnectState();
}

Login::~Login() {
  net::NetworkChangeNotifier::RemoveConnectionTypeObserver(this);
  net::NetworkChangeNotifier::RemoveIPAddressObserver(this);
}

void Login::StartConnection() {
  VLOG(1) << "Starting connection...";
#if !defined(OPERA_SYNC)
  single_attempt_.reset(new SingleLoginAttempt(login_settings_, this));
#else  // defined(OPERA_SYNC)
  single_attempt_.reset(
      new SingleLoginAttempt(login_settings_, this, token_generator_));
#endif
}

void Login::UpdateXmppSettings(const buzz::XmppClientSettings& user_settings) {
  VLOG(1) << "XMPP settings updated";
  login_settings_.set_user_settings(user_settings);
}

#if defined(OPERA_SYNC)
void Login::SetTokenGenerator(
    const opera::GenerateTokenCallback& token_generator) {
  token_generator_ = token_generator;
}
#endif  // OPERA_SYNC

// In the code below, we assume that calling a delegate method may end
// up in ourselves being deleted, so we always call it last.
//
// TODO(akalin): Add unit tests to enforce the behavior above.

void Login::OnConnect(base::WeakPtr<buzz::XmppTaskParentInterface> base_task) {
  VLOG(1) << "Connected";
  ResetReconnectState();
  delegate_->OnConnect(base_task);
}

void Login::OnRedirect(const ServerInformation& redirect_server) {
  VLOG(1) << "Redirected";
  login_settings_.SetRedirectServer(redirect_server);
  // Drop the current connection, and start the login process again.
  StartConnection();
  delegate_->OnTransientDisconnection();
}

#if defined(OPERA_SYNC)
void Login::OnCredentialsRejected(opera_sync::OperaAuthProblem auth_problem) {
#else
void Login::OnCredentialsRejected() {
#endif  // OPERA_SYNC
  VLOG(1) << "Credentials rejected";
#if defined(OPERA_SYNC)
// NOTE: In Opera port we don't schedule a reconnect here as it is
// quite possible that the auth header generation request during the
// reconnection attempt will be done while the token is still being
// refreshed and therefore we will eventually DCHECK and send an auth
// header with an empty token.
// We do count that XmppPushClient::UpdateCredentials() will trigger
// another connection attempt.
#else
  TryReconnect();
#endif  // OPERA_SYNC
#if defined(OPERA_SYNC)
  delegate_->OnCredentialsRejected(auth_problem);
#else
  delegate_->OnCredentialsRejected();
#endif  // OPERA_SYNC
}

#if defined(OPERA_SYNC)
void Login::OnAuthDataUnavailable() {
  // The OAuth token was empty or there was no account service.
  // Either way we can't obtain a complete auth header at this
  // moment. Schedule another reconnection attempt.
  VLOG(1) << "Auth data unavailable";
  TryReconnect();
  delegate_->OnAuthDataUnavailable();
}
#endif  // OPERA_SYNC

void Login::OnSettingsExhausted() {
  VLOG(1) << "Settings exhausted";
  TryReconnect();
  delegate_->OnTransientDisconnection();
}

void Login::OnIPAddressChanged() {
  VLOG(1) << "IP address changed";
  OnNetworkEvent();
}

void Login::OnConnectionTypeChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  VLOG(1) << "Connection type changed";
  OnNetworkEvent();
}

void Login::OnDNSChanged() {
  VLOG(1) << "DNS changed";
  OnNetworkEvent();
}

void Login::OnNetworkEvent() {
  // Reconnect in 1 to 9 seconds (vary the time a little to try to
  // avoid spikey behavior on network hiccups).
  reconnect_interval_ = base::TimeDelta::FromSeconds(base::RandInt(1, 9));
  TryReconnect();
  delegate_->OnTransientDisconnection();
}

void Login::ResetReconnectState() {
  reconnect_interval_ =
      base::TimeDelta::FromSeconds(base::RandInt(5, 25));
  reconnect_timer_.Stop();
}

void Login::TryReconnect() {
  DCHECK_GT(reconnect_interval_.InSeconds(), 0);
  single_attempt_.reset();
  reconnect_timer_.Stop();
  VLOG(1) << "Reconnecting in "
           << reconnect_interval_.InSeconds() << " seconds";
  reconnect_timer_.Start(
      FROM_HERE, reconnect_interval_, this, &Login::DoReconnect);
}

void Login::DoReconnect() {
  // Double reconnect time up to 30 minutes.
  const base::TimeDelta kMaxReconnectInterval =
      base::TimeDelta::FromMinutes(30);
  reconnect_interval_ *= 2;
  if (reconnect_interval_ > kMaxReconnectInterval)
    reconnect_interval_ = kMaxReconnectInterval;
  VLOG(1) << "Reconnecting...";
  StartConnection();
}

}  // namespace notifier
