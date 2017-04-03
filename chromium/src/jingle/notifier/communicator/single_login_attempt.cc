// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "jingle/notifier/communicator/single_login_attempt.h"

#include <stdint.h>

#include <limits>
#include <string>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "jingle/notifier/base/const_communicator.h"
#include "jingle/notifier/base/gaia_token_pre_xmpp_auth.h"
#include "jingle/notifier/listener/xml_element_util.h"
#include "net/base/host_port_pair.h"
#include "third_party/webrtc/libjingle/xmllite/xmlelement.h"
#include "webrtc/libjingle/xmpp/constants.h"
#include "webrtc/libjingle/xmpp/xmppclientsettings.h"

#if defined(OPERA_SYNC)
#include "base/bind.h"
#include "jingle/notifier/base/opera_token_pre_xmpp_auth.h"
#include "third_party/re2/src/re2/re2.h"
#endif  // defined(OPERA_SYNC)

#if defined(OPERA_SYNC)
const std::string ExtractBareTokenFromAuthHeader(const std::string& header) {
  const std::string re("oauth_token=\"(.+)\",");
  std::string token;
  if (!RE2::PartialMatch(header, re, &token)) {
    LOG(WARNING) << "Cannot extract token from auth header! " << header;
  }
  return token;
}

#endif  // OPERA_SYNC

namespace notifier {

SingleLoginAttempt::Delegate::~Delegate() {}

#if !defined(OPERA_SYNC)
SingleLoginAttempt::SingleLoginAttempt(const LoginSettings& login_settings,
                                       Delegate* delegate)
    : login_settings_(login_settings),
      delegate_(delegate),
      settings_list_(
          MakeConnectionSettingsList(login_settings_.GetServers(),
                                     login_settings_.try_ssltcp_first())),
      current_settings_(settings_list_.begin()),
      weak_ptr_factory_(this) {
  if (settings_list_.empty()) {
    NOTREACHED();
    return;
  }
  TryConnect(*current_settings_);
}
#else
SingleLoginAttempt::SingleLoginAttempt(
    const LoginSettings& login_settings,
    Delegate* delegate,
    const opera::GenerateTokenCallback& token_generator)
    : token_generator_(token_generator),
      login_settings_(login_settings),
      delegate_(delegate),
      settings_list_(
          MakeConnectionSettingsList(login_settings_.GetServers(),
                                     login_settings_.try_ssltcp_first())),
      current_settings_(settings_list_.begin()),
      weak_ptr_factory_(this) {
  if (settings_list_.empty()) {
    NOTREACHED();
    return;
  }
  if (token_generator.is_null()) {
    // Opera OAuth2
    TryConnect(*current_settings_);
  } else {
    // Update the token upon each new connection attempt. The token contains a
    // nonce and timestamp which need to be unique, so we can't reuse old
    // tokens.
    token_generator_.Run(
        base::Bind(&SingleLoginAttempt::UpdateTokenAndTryConnect,
                   weak_ptr_factory_.GetWeakPtr()));
  }
}
#endif  // OPERA_SYNC

SingleLoginAttempt::~SingleLoginAttempt() {}

// In the code below, we assume that calling a delegate method may end
// up in ourselves being deleted, so we always call it last.
//
// TODO(akalin): Add unit tests to enforce the behavior above.

void SingleLoginAttempt::OnConnect(
    base::WeakPtr<buzz::XmppTaskParentInterface> base_task) {
  DVLOG(1) << "Connected to " << current_settings_->ToString();
  delegate_->OnConnect(base_task);
}

namespace {

// This function is more permissive than
// net::HostPortPair::FromString().  If the port is missing or
// unparseable, it assumes the default XMPP port.  The hostname may be
// empty.
net::HostPortPair ParseRedirectText(const std::string& redirect_text) {
  std::vector<std::string> parts = base::SplitString(
      redirect_text, ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  net::HostPortPair redirect_server;
  redirect_server.set_port(kDefaultXmppPort);
  if (parts.empty()) {
    return redirect_server;
  }
  redirect_server.set_host(parts[0]);
  if (parts.size() <= 1) {
    return redirect_server;
  }
  // Try to parse the port, falling back to kDefaultXmppPort.
  int port = kDefaultXmppPort;
  if (!base::StringToInt(parts[1], &port)) {
    port = kDefaultXmppPort;
  }
  if (port <= 0 || port > std::numeric_limits<uint16_t>::max()) {
    port = kDefaultXmppPort;
  }
  redirect_server.set_port(port);
  return redirect_server;
}

}  // namespace

void SingleLoginAttempt::OnError(buzz::XmppEngine::Error error, int subcode,
                                 const buzz::XmlElement* stream_error) {
  VLOG(1) << "Error: " << error << ", subcode: " << subcode
           << (stream_error
                   ? (", stream error: " + XmlElementToString(*stream_error))
                   : std::string());

  DCHECK_EQ(error == buzz::XmppEngine::ERROR_STREAM, stream_error != NULL);

  // Check for redirection.  We expect something like:
  //
  // <stream:error><see-other-host xmlns="urn:ietf:params:xml:ns:xmpp-streams"/><str:text xmlns:str="urn:ietf:params:xml:ns:xmpp-streams">talk.google.com</str:text></stream:error> [2]
  //
  // There are some differences from the spec [1]:
  //
  //   - we expect a separate text element with the redirection info
  //     (which is the format Google Talk's servers use), whereas the
  //     spec puts the redirection info directly in the see-other-host
  //     element;
  //   - we check for redirection only during login, whereas the
  //     server can send down a redirection at any time according to
  //     the spec. (TODO(akalin): Figure out whether we need to handle
  //     redirection at any other point.)
  //
  // [1]: http://xmpp.org/internet-drafts/draft-saintandre-rfc3920bis-08.html#streams-error-conditions-see-other-host
  // [2]: http://forums.miranda-im.org/showthread.php?24376-GoogleTalk-drops
  if (stream_error) {
    const buzz::XmlElement* other =
        stream_error->FirstNamed(buzz::QN_XSTREAM_SEE_OTHER_HOST);
    if (other) {
      const buzz::XmlElement* text =
          stream_error->FirstNamed(buzz::QN_XSTREAM_TEXT);
      if (text) {
        // Yep, its a "stream:error" with "see-other-host" text,
        // let's parse out the server:port, and then reconnect
        // with that.
        const net::HostPortPair& redirect_server =
            ParseRedirectText(text->BodyText());
        // ParseRedirectText shouldn't return a zero port.
        DCHECK_NE(redirect_server.port(), 0u);
        // If we don't have a host, ignore the redirection and treat
        // it like a regular error.
        if (!redirect_server.host().empty()) {
          delegate_->OnRedirect(
              ServerInformation(
                  redirect_server,
                  current_settings_->ssltcp_support));
          // May be deleted at this point.
          return;
        }
      }
    }
  }

  if (error == buzz::XmppEngine::ERROR_UNAUTHORIZED) {
    VLOG(1) << "Credentials rejected";
#if defined(OPERA_SYNC)
    opera_sync::OperaAuthProblem problem;
    problem.set_auth_errcode(subcode);
    problem.set_token(ExtractBareTokenFromAuthHeader(
        login_settings_.user_settings().auth_token()));
    delegate_->OnCredentialsRejected(problem);
#else
    delegate_->OnCredentialsRejected();
#endif  // OPERA_SYNC
    return;
  }

  if (error == buzz::XmppEngine::ERROR_TEMPORARY_AUTH_FAILURE) {
    // This handles the <temporary-auth-failure/> signal.
    // This is expected in case the push service has any problems
    // in regard to giving a definite answer to whether the token
    // supplied is valid, i.e. there is an internal push service problem,
    // there is an internal auth service problem or the auth service is
    // not reachable at all at the time.
    // This is considered a temporary condition and is followed by recurring
    // reconnection attempts.
    VLOG(1) << "Temporary auth failure";
    // Continue with advancing the settings and TryConnect().
  }

  if (current_settings_ == settings_list_.end()) {
    NOTREACHED();
    return;
  }

  ++current_settings_;
  if (current_settings_ == settings_list_.end()) {
    VLOG(1) << "Could not connect to any XMPP server";
    delegate_->OnSettingsExhausted();
    return;
  }

#if OPERA_SYNC
  if (token_generator_.is_null()) {
    // Opera OAuth2
    TryConnect(*current_settings_);
  } else {
    token_generator_.Run(
        base::Bind(&SingleLoginAttempt::UpdateTokenAndTryConnect,
                   weak_ptr_factory_.GetWeakPtr()));
  }
#else
  TryConnect(*current_settings_);
#endif  // OPERA_SYNC
}

void SingleLoginAttempt::TryConnect(
    const ConnectionSettings& connection_settings) {
  DVLOG(1) << "Trying to connect to " << connection_settings.ToString();
  // Copy the user settings and fill in the connection parameters from
  // |connection_settings|.
  buzz::XmppClientSettings client_settings = login_settings_.user_settings();
  connection_settings.FillXmppClientSettings(&client_settings);

#if !defined(OPERA_SYNC)
  buzz::Jid jid(client_settings.user(), client_settings.host(),
                buzz::STR_EMPTY);
#endif  // !defined(OPERA_SYNC)
  buzz::PreXmppAuth* pre_xmpp_auth =
#if defined(OPERA_SYNC)
      new OperaTokenPreXmppAuth(
          client_settings.auth_token(),
          login_settings_.auth_mechanism());
#else
      new GaiaTokenPreXmppAuth(
          jid.Str(), client_settings.auth_token(),
          client_settings.token_service(),
          login_settings_.auth_mechanism());
#endif  // defined(OPERA_SYNC)
  xmpp_connection_.reset(
      new XmppConnection(client_settings,
                         login_settings_.request_context_getter(),
                         this,
                         pre_xmpp_auth));
}

#if defined(OPERA_SYNC)
void SingleLoginAttempt::UpdateTokenAndTryConnect(
    const std::string& new_token) {
  if (new_token.empty()) {
    // This can only happen during application shutdown, in this case don't
    // attempt to start the connection with an empty token.
    // Note DNA-48030, this can also happen in poor network conditions.
    delegate_->OnAuthDataUnavailable();
    return;
  }
  buzz::XmppClientSettings settings = login_settings_.user_settings();
  settings.set_auth_token(settings.auth_mechanism(), new_token);
  login_settings_.set_user_settings(settings);
  VLOG(1) << "Generated a new token for pusher: " << new_token;
  TryConnect(*current_settings_);
}
#endif  // defined(OPERA_SYNC)

}  // namespace notifier
