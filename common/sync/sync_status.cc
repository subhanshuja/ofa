// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sync_status.h"

#include "base/strings/string_number_conversions.h"
#include "net/base/net_errors.h"

#include "common/sync/sync_config.h"

namespace {
const std::string BoolToString(bool in, const std::string& name) {
  return (in ? (name + "; ") : "");
}

const std::string IntToString(int in, const std::string& name) {
  return (in ? (name + "=" + base::IntToString(in) + "; ") : "");
}

const std::string TokenStateToString(
    opera::SyncStatus::TokenState token_state) {
  switch (token_state) {
    case opera::SyncStatus::TS_UNSET:
      return "UNSET";
    case opera::SyncStatus::TS_OK:
      return "OK";
    case opera::SyncStatus::TS_WARNING:
      return "WARNING";
    case opera::SyncStatus::TS_ERROR:
      return "ERROR";
    case opera::SyncStatus::TS_LOGGED_OUT:
      return "LOGGED_OUT";
    default:
      NOTREACHED();
      return "<UNKNOWN>";
  }
}

const std::string ButtonStateToString(opera::SyncStatus::ButtonState state) {
  switch (state) {
    case opera::SyncStatus::BS_INACTIVE:
      return "INACTIVE";
    case opera::SyncStatus::BS_OK:
      return "OK";
    case opera::SyncStatus::BS_WARNING:
      return "WARNING";
    case opera::SyncStatus::BS_ERROR:
      return "ERROR";
    case opera::SyncStatus::BS_INFO:
      return "INFO";
    default:
      NOTREACHED();
      return "<UNKNOWN>";
  }
}
}  // namespace

namespace opera {

SyncStatus::SyncStatus(int network_error,
                       const SyncServerInfo& server_info,
                       bool busy,
                       bool logged_in,
                       bool internal_error,
                       const std::string& internal_error_message,
                       bool disable_sync,
                       bool passphrase_required,
                       bool setup_in_progress,
                       syncer::ModelTypeSet unconfigured_types)
    : network_error_(network_error),
      server_info_(server_info),
      busy_(busy),
      logged_in_(logged_in),
      internal_error_(internal_error),
      internal_error_message_(internal_error_message),
      disable_sync_(disable_sync),
      passphrase_required_(passphrase_required),
      setup_in_progress_(setup_in_progress),
      unconfigured_types_(unconfigured_types),
      is_configuring_(false),
      is_any_type_configured_(false),
      request_login_again_(false),
      token_state_(TS_UNSET),
      auth_error_(GoogleServiceAuthError::NONE) {}

SyncStatus::SyncStatus(const SyncStatus&) = default;

SyncStatus::~SyncStatus() {}

bool SyncStatus::operator==(const SyncStatus& other) const {
  return network_error_code() == other.network_error_code() &&
         server_error_code() == other.server_error_code() &&
         server_message() == other.server_message() && busy() == other.busy() &&
         logged_in() == other.logged_in() &&
         internal_error() == other.internal_error() &&
         internal_error_message() == other.internal_error_message() &&
         disable_sync() == other.disable_sync() &&
         passphrase_required() == other.passphrase_required() &&
         setup_in_progress() == other.setup_in_progress() &&
         unconfigured_types() == other.unconfigured_types() &&
         is_configuring() == other.is_configuring() &&
         first_start() == other.first_start() &&
         request_login_again() == other.request_login_again() &&
         token_state() == other.token_state();
}

bool SyncStatus::operator!=(const SyncStatus& other) const {
  return !(*this == other);
}

bool SyncStatus::enabled() const {
  // From the UI point of view sync is "enabled" starting from the moment the
  // account becomes logged in.
  // It is no longer enabled the moment user logs out or error happens.
  // This will probably need refining within the error handling.
  return logged_in() && !internal_error() &&
         server_error_code() != SyncServerInfo::SYNC_UNKNOWN_ERROR;
}

bool SyncStatus::busy() const {
  return busy_;
}

bool SyncStatus::network_error() const {
  return network_error_code() != net::OK;
}

bool SyncStatus::server_error() const {
  return server_error_code() != SyncServerInfo::SYNC_UNKNOWN &&
         server_error_code() != SyncServerInfo::SYNC_OK &&
         server_error_code() != SyncServerInfo::SYNC_AUTH_TOKEN_EXPIRED;
}

int SyncStatus::server_error_code() const {
  return server_info_.server_status;
}

const std::string& SyncStatus::server_message() const {
  return server_info_.server_message;
}

bool SyncStatus::internal_error() const {
  return internal_error_;
}

bool SyncStatus::token_error() const {
  return token_state() == TS_ERROR;
}

bool SyncStatus::token_warning() const {
  return token_state() == TS_WARNING;
}

const std::string& SyncStatus::internal_error_message() const {
  return internal_error_message_;
}

bool SyncStatus::disable_sync() const {
  return disable_sync_;
}

bool SyncStatus::first_start() const {
  return is_any_type_configured_;
}

void SyncStatus::set_first_start(bool is_any_type_configured) {
  is_any_type_configured_ = is_any_type_configured;
}

bool SyncStatus::has_unconfigured_types() const {
  return !unconfigured_types_.Empty();
}

bool SyncStatus::is_configuring() const {
  return is_configuring_;
}

void SyncStatus::set_is_configuring(bool is_configuring) {
  is_configuring_ = is_configuring;
}

syncer::ModelTypeSet SyncStatus::unconfigured_types() const {
  return unconfigured_types_;
}

SyncStatus::ButtonState SyncStatus::button_state() const {
  bool any_errors = server_error() || network_error() || internal_error();
  bool is_enabled = enabled();
  bool has_passphrase_problem = false;
  bool is_first_start = false;
  bool unconfigured_types = false;
  bool it_is_configuring = false;
  bool log_in_again = false;
  bool temporary_token_problem = false;
  bool fatal_token_problem = false;

  if (opera::SyncConfig::ShouldUseAuthTokenRecovery() ||
      opera::SyncConfig::UsesOAuth2()) {
    temporary_token_problem = token_warning() || has_auth_error();
    fatal_token_problem = token_error();
  }

  if (opera::SyncConfig::ShouldSynchronizePasswords()) {
    has_passphrase_problem = passphrase_required();
    is_first_start = first_start();
    unconfigured_types = has_unconfigured_types();
    it_is_configuring = is_configuring();
    log_in_again = request_login_again();
  }

  const bool is_oauth2 = opera::SyncConfig::UsesOAuth2();

  if (fatal_token_problem || (any_errors && !temporary_token_problem)) {
    return BS_ERROR;
  } else if (!is_enabled) {
    return BS_INACTIVE;
  } else if (!is_oauth2 &&
             (!it_is_configuring && (has_passphrase_problem || log_in_again ||
                                     temporary_token_problem))) {
    return BS_WARNING;
  } else if (is_oauth2 && (temporary_token_problem ||
                           (!it_is_configuring &&
                            (has_passphrase_problem || log_in_again)))) {
    return BS_WARNING;
  } else if (is_first_start || (!it_is_configuring && unconfigured_types)) {
    return BS_INFO;
  } else {
    return BS_OK;
  }
}

bool SyncStatus::request_login_again() const {
  return request_login_again_;
}

void SyncStatus::set_request_login_again(bool login_again) {
  request_login_again_ = login_again;
}

bool SyncStatus::can_show_advanced_config() const {
  // This method is used to check whether the "Advanced sync configuration"
  // button should be enabled, and also to close the dialog immediately in
  // case it was opened directly via the address bar
  // (chrome://settings/syncSetup).
  // We must allow opening the advanced config in the first-start state
  // since the user can use the "Choose what to sync" link in the popup
  // then.
  if (opera::SyncConfig::UsesOAuth2()) {
    return first_start() ||
           (!is_configuring() && !internal_error() && !request_login_again() &&
            !server_error() && !auth_error());
  } else {
    return first_start() || (!is_configuring() && !internal_error() &&
                             !request_login_again() && !server_error());
  }
}

void SyncStatus::set_token_state(TokenState token_state) {
  token_state_ = token_state;
}

SyncStatus::TokenState SyncStatus::token_state() const {
  return token_state_;
}

void SyncStatus::set_auth_error(GoogleServiceAuthError::State auth_error) {
  auth_error_ = auth_error;
}

GoogleServiceAuthError::State SyncStatus::auth_error() const {
  return auth_error_;
}

bool SyncStatus::has_auth_error() const {
  return auth_error_ != GoogleServiceAuthError::NONE;
}

std::string SyncStatus::ToString() const {
  std::string out;
  out += BoolToString(busy(), "busy");
  out += BoolToString(can_show_advanced_config(), "can_show_advanced_config");
  out += BoolToString(enabled(), "enabled");
  out += BoolToString(first_start(), "first_start");
  out += BoolToString(internal_error(), "internal_error");
  out += BoolToString(disable_sync(), "disable_sync");
  out += BoolToString(is_configuring(), "is_configuring");
  out += BoolToString(logged_in(), "logged_in");
  out += BoolToString(network_error(), "network_error");
  out += BoolToString(passphrase_required(), "passphrase_required");
  out += BoolToString(server_error(), "server_error");
  out += BoolToString(setup_in_progress(), "setup_in_progress");
  out += BoolToString(request_login_again(), "request_login_again");
  out += BoolToString(has_unconfigured_types(), "has_unconfigured_types");
  out += BoolToString(has_auth_error(), "has_auth_error");
  out += IntToString(auth_error(), "auth_error");
  out += unconfigured_types().Empty()
             ? ""
             : ("unconfigured_types=[" +
                syncer::ModelTypeSetToString(unconfigured_types()) + "]; ");
  out += "bs=" + ButtonStateToString(button_state()) + "; ";
  out += "ts=" + TokenStateToString(token_state());
  return out;
}

}  // namespace opera
