// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "jingle/notifier/base/opera_token_pre_xmpp_auth.h"

#include <algorithm>

#include "base/logging.h"
#include "webrtc/base/socketaddress.h"
#include "webrtc/libjingle/xmpp/constants.h"
#include "webrtc/libjingle/xmpp/saslcookiemechanism.h"

namespace notifier {

namespace {

class OperaMechanism : public buzz::SaslMechanism {
 public:
  OperaMechanism(
      const std::string& mechanism,
      const std::string& token)
      : mechanism_(mechanism), token_(token) {}

  ~OperaMechanism() override {}

  buzz::XmlElement* StartSaslAuth() override {
    buzz::XmlElement* auth = buzz::SaslMechanism::StartSaslAuth();
    auth->SetAttr(buzz::QN_SASL_MECHANISM, mechanism_);
    auth->AddText(create_credentials());
    return auth;
  }

  // buzz::SaslMechanism implementation
  std::string GetMechanismName() override { return mechanism_; }

 private:
  std::string mechanism_;
  std::string token_;

  std::string create_credentials() const {
    // Currently we use Base64-encoded OAuth header to authenticate.
    // It is passed though pipelines intended for token to avoid
    // developing parallel mechanism just for this header.
    const std::string& oauth_header = token_;
    std::string encoded_oauth_header = Base64Encode(oauth_header);
    return encoded_oauth_header;
  }

  DISALLOW_COPY_AND_ASSIGN(OperaMechanism);
};

}  // namespace

OperaTokenPreXmppAuth::OperaTokenPreXmppAuth(
    const std::string& token,
    const std::string& auth_mechanism)
    : token_(token), auth_mechanism_(auth_mechanism) {
  DCHECK(!auth_mechanism_.empty());
}

OperaTokenPreXmppAuth::~OperaTokenPreXmppAuth() {}

void OperaTokenPreXmppAuth::StartPreXmppAuth(
    const buzz::Jid&,
    const rtc::SocketAddress&,
    const rtc::CryptString&,
    const std::string&,
    const std::string&) {
  SignalAuthDone();
}

bool OperaTokenPreXmppAuth::IsAuthDone() const {
  return true;
}

bool OperaTokenPreXmppAuth::IsAuthorized() const {
  return true;
}

bool OperaTokenPreXmppAuth::HadError() const {
  return false;
}

int OperaTokenPreXmppAuth::GetError() const {
  return 0;
}

buzz::CaptchaChallenge OperaTokenPreXmppAuth::GetCaptchaChallenge() const {
  return buzz::CaptchaChallenge();
}

std::string OperaTokenPreXmppAuth::GetAuthToken() const {
  return token_;
}

std::string OperaTokenPreXmppAuth::GetAuthMechanism() const {
  return auth_mechanism_;
}

std::string OperaTokenPreXmppAuth::ChooseBestSaslMechanism(
    const std::vector<std::string>& mechanisms,
    bool encrypted) {
  return (std::find(mechanisms.begin(), mechanisms.end(), auth_mechanism_) !=
          mechanisms.end())
             ? auth_mechanism_
             : std::string();
}

buzz::SaslMechanism* OperaTokenPreXmppAuth::CreateSaslMechanism(
    const std::string& mechanism) {
  if (mechanism == auth_mechanism_)
    return new OperaMechanism(auth_mechanism_, token_);
  return NULL;
}

}  // namespace notifier
