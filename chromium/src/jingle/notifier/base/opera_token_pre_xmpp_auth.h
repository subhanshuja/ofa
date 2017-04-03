// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef JINGLE_NOTIFIER_BASE_OPERA_TOKEN_PRE_XMPP_AUTH_H_
#define JINGLE_NOTIFIER_BASE_OPERA_TOKEN_PRE_XMPP_AUTH_H_

#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "webrtc/libjingle/xmpp/prexmppauth.h"

namespace notifier {

// This class implements buzz::PreXmppAuth interface for auth-opera.com based
// authentication. It looks for the X-OPERA-TOKEN auth mechanism and uses that
// instead of the default auth mechanism (PLAIN).
//
// It is assumed that token is managed by SyncServices and here we merely use it
// to log in the XMPP client. We don't need to handle token change since it will
// simply make client disconnect and authenticate again with new token.
class OperaTokenPreXmppAuth : public buzz::PreXmppAuth {
 public:
  OperaTokenPreXmppAuth(const std::string& token,
                        const std::string& auth_mechanism);

  ~OperaTokenPreXmppAuth() override;

  // buzz::PreXmppAuth (-buzz::SaslHandler) implementation.  We stub
  // all the methods out as we don't actually do any authentication at
  // this point.
  void StartPreXmppAuth(const buzz::Jid& jid,
                        const rtc::SocketAddress& server,
                        const rtc::CryptString& pass,
                        const std::string& auth_mechanism,
                        const std::string& auth_token) override;

  bool IsAuthDone() const override;

  bool IsAuthorized() const override;

  bool HadError() const override;

  int GetError() const override;

  buzz::CaptchaChallenge GetCaptchaChallenge() const override;

  std::string GetAuthToken() const override;

  std::string GetAuthMechanism() const override;

  // buzz::SaslHandler implementation.

  std::string ChooseBestSaslMechanism(
      const std::vector<std::string>& mechanisms, bool encrypted) override;

  buzz::SaslMechanism* CreateSaslMechanism(
      const std::string& mechanism) override;

 private:
  std::string username_;
  std::string token_;
  std::string token_service_;
  std::string auth_mechanism_;
};

}  // namespace notifier

#endif  // JINGLE_NOTIFIER_BASE_OPERA_TOKEN_PRE_XMPP_AUTH_H_
