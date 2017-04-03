// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef JINGLE_NOTIFIER_COMMUNICATOR_SINGLE_LOGIN_ATTEMPT_H_
#define JINGLE_NOTIFIER_COMMUNICATOR_SINGLE_LOGIN_ATTEMPT_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "jingle/notifier/base/xmpp_connection.h"
#include "jingle/notifier/communicator/connection_settings.h"
#include "jingle/notifier/communicator/login_settings.h"
#include "webrtc/libjingle/xmpp/xmppengine.h"

#if defined(OPERA_SYNC)
#include <string>
#include "components/sync/util/opera_auth_problem.h"
#include "jingle/notifier/base/token_generator.h"
#endif  // OPERA_SYNC

namespace buzz {
class XmppTaskParentInterface;
}  // namespace buzz

namespace notifier {

struct ServerInformation;

// Handles all of the aspects of a single login attempt.  By
// containing this within one class, when another login attempt is
// made, this class can be destroyed to immediately stop the previous
// login attempt.
class SingleLoginAttempt : public XmppConnection::Delegate {
 public:
  // At most one delegate method will be called, depending on the
  // result of the login attempt.  After the delegate method is
  // called, this class won't do anything anymore until it is
  // destroyed, at which point it will disconnect if necessary.
  class Delegate {
   public:
    // Called when the login attempt is successful.
    virtual void OnConnect(
        base::WeakPtr<buzz::XmppTaskParentInterface> base_task) = 0;

    // Called when the server responds with a redirect.  A new login
    // attempt should be made to the given redirect server.
    virtual void OnRedirect(const ServerInformation& redirect_server) = 0;

    // Called when a server rejects the client's login credentials.  A
    // new login attempt should be made once the client provides new
    // credentials.
#if defined(OPERA_SYNC)
    virtual void OnCredentialsRejected(opera_sync::OperaAuthProblem) = 0;
    virtual void OnAuthDataUnavailable() = 0;
#else
    virtual void OnCredentialsRejected() = 0;
#endif  // OPERA_SYNC

    // Called when no server could be logged into for reasons other
    // than redirection or rejected credentials.  A new login attempt
    // may be created, but it should be done with exponential backoff.
    virtual void OnSettingsExhausted() = 0;

   protected:
    virtual ~Delegate();
  };

  // Does not take ownership of |delegate|, which must not be NULL.
#if !defined(OPERA_SYNC)
  SingleLoginAttempt(const LoginSettings& login_settings, Delegate* delegate);
#else
  SingleLoginAttempt(const LoginSettings& login_settings,
                     Delegate* delegate,
                     const opera::GenerateTokenCallback& token_generator);
#endif  // OPERA_SYNC

  ~SingleLoginAttempt() override;

  // XmppConnection::Delegate implementation.
  void OnConnect(base::WeakPtr<buzz::XmppTaskParentInterface> parent) override;
  void OnError(buzz::XmppEngine::Error error,
               int error_subcode,
               const buzz::XmlElement* stream_error) override;

 private:
  void TryConnect(const ConnectionSettings& new_settings);

#if defined(OPERA_SYNC)
  void UpdateTokenAndTryConnect(const std::string& new_token);

  // Generates new signed auth token.
  opera::GenerateTokenCallback token_generator_;
#endif  // OPERA_SYNC

#if !defined(OPERA_SYNC)
  const LoginSettings login_settings_;
#else
  // We modify LoginSettings in this class when we generate new Auth header.
  // This is needed because every message sent to the server must be signed
  // by a newly generated header, with a fresh nonce and timestamp.
  LoginSettings login_settings_;
#endif
  Delegate* const delegate_;
  const ConnectionSettingsList settings_list_;
  ConnectionSettingsList::const_iterator current_settings_;
  std::unique_ptr<XmppConnection> xmpp_connection_;

  // Used only by Opera-specific code for safe returns from PostTasks, but
  // added without ifdefs because it doesn't hurt and it makes the code less
  // cluttered.
  base::WeakPtrFactory<SingleLoginAttempt> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SingleLoginAttempt);
};

}  // namespace notifier

#endif  // JINGLE_NOTIFIER_COMMUNICATOR_SINGLE_LOGIN_ATTEMPT_H_
