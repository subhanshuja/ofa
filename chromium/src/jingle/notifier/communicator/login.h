// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef JINGLE_NOTIFIER_COMMUNICATOR_LOGIN_H_
#define JINGLE_NOTIFIER_COMMUNICATOR_LOGIN_H_

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "jingle/notifier/base/server_information.h"
#include "jingle/notifier/communicator/login_settings.h"
#include "jingle/notifier/communicator/single_login_attempt.h"
#include "net/base/network_change_notifier.h"
#include "webrtc/libjingle/xmpp/xmppengine.h"

#if defined(OPERA_SYNC)
#include "components/sync/util/opera_auth_problem.h"
#include "jingle/notifier/base/token_generator.h"
#endif  // OPERA_SYNC

namespace buzz {
class XmppClient;
class XmppClientSettings;
class XmppTaskParentInterface;
}  // namespace buzz

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace notifier {

class LoginSettings;

// Does the login, keeps it alive (with refreshing cookies and
// reattempting login when disconnected), and figures out what actions
// to take on the various errors that may occur.
//
// TODO(akalin): Make this observe proxy config changes also.
class Login : public net::NetworkChangeNotifier::IPAddressObserver,
              public net::NetworkChangeNotifier::ConnectionTypeObserver,
              public net::NetworkChangeNotifier::DNSObserver,
              public SingleLoginAttempt::Delegate {
 public:
  class Delegate {
   public:
    // Called when a connection has been successfully established.
    virtual void OnConnect(
        base::WeakPtr<buzz::XmppTaskParentInterface> base_task) = 0;

    // Called when there's no connection to the server but we expect
    // it to come back come back eventually.  The connection will be
    // retried with exponential backoff.
    virtual void OnTransientDisconnection() = 0;

    // Called when the current login credentials have been rejected.
    // The connection will still be retried with exponential backoff;
    // it's up to the delegate to stop connecting and/or prompt for
    // new credentials.
#if defined(OPERA_SYNC)
    virtual void OnCredentialsRejected(opera_sync::OperaAuthProblem) = 0;
    virtual void OnAuthDataUnavailable() = 0;
#else
    virtual void OnCredentialsRejected() = 0;
#endif  // OPERA_SYNC

   protected:
    virtual ~Delegate();
  };

  // Does not take ownership of |delegate|, which must not be NULL.
  Login(Delegate* delegate,
        const buzz::XmppClientSettings& user_settings,
        const scoped_refptr<net::URLRequestContextGetter>&
            request_context_getter,
        const ServerList& servers,
        bool try_ssltcp_first,
        const std::string& auth_mechanism);
  ~Login() override;

  // Starts connecting (or forces a reconnection if we're backed off).
  void StartConnection();

  // The updated settings take effect only the next time when a
  // connection is attempted (either via reconnection or a call to
  // StartConnection()).
  void UpdateXmppSettings(const buzz::XmppClientSettings& user_settings);

  // net::NetworkChangeNotifier::IPAddressObserver implementation.
  void OnIPAddressChanged() override;

  // net::NetworkChangeNotifier::ConnectionTypeObserver implementation.
  void OnConnectionTypeChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  // net::NetworkChangeNotifier::DNSObserver implementation.
  void OnDNSChanged() override;

  // SingleLoginAttempt::Delegate implementation.
  void OnConnect(
      base::WeakPtr<buzz::XmppTaskParentInterface> base_task) override;
  void OnRedirect(const ServerInformation& redirect_server) override;
#if defined(OPERA_SYNC)
  void OnCredentialsRejected(opera_sync::OperaAuthProblem info) override;
  void OnAuthDataUnavailable() override;
#else
  void OnCredentialsRejected() override;
#endif  // OPERA_SYNC
  void OnSettingsExhausted() override;

#if defined(OPERA_SYNC)
  // If a token generator is set, it will be used to generate new oauth tokens
  // for every new connection attempt instead of reusing the token present in
  // login_settings_. Opera's protocol requires a fresh nonce and timestamp
  // to be present within the token for security, so reusing old ones doesn't
  // work.
  void SetTokenGenerator(const opera::GenerateTokenCallback& token_generator);
#endif  // OPERA_SYNC

 private:
  // Called by the various network notifications.
  void OnNetworkEvent();

  // Stops any existing reconnect timer and sets an initial reconnect
  // interval.
  void ResetReconnectState();

  // Tries to reconnect in some point in the future.  If called
  // repeatedly, will wait longer and longer until reconnecting.
  void TryReconnect();

  // The actual function (called by |reconnect_timer_|) that does the
  // reconnection.
  void DoReconnect();

#if defined(OPERA_SYNC)
  void UpdateTokenAndStartConnection(const std::string& new_token);
#endif  // OPERA_SYNC

  Delegate* const delegate_;
  LoginSettings login_settings_;
  std::unique_ptr<SingleLoginAttempt> single_attempt_;

  // reconnection state.
  base::TimeDelta reconnect_interval_;
  base::OneShotTimer reconnect_timer_;

#if defined(OPERA_SYNC)
  // Generates new signed auth token.
  opera::GenerateTokenCallback token_generator_;
#endif  // OPERA_SYNC

  DISALLOW_COPY_AND_ASSIGN(Login);
};

}  // namespace notifier

#endif  // JINGLE_NOTIFIER_COMMUNICATOR_LOGIN_H_
