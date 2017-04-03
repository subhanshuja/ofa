// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sync_auth_keeper_util.h"

#include <string>

#include "components/sync/api/time.h"

#include "common/sync/pref_names.h"

namespace opera {
namespace sync {

const char kNotAvailableString[] = "n/a";

LastSessionDescription::LastSessionDescription(PrefService* pref_service)
    : duration_(base::TimeDelta::FromMicroseconds(0)),
      id_(std::string()),
      logout_reason_(account_util::LR_UNKNOWN),
      pref_service_(pref_service) {
  Load();
}

LastSessionDescription::~LastSessionDescription() {
  Save();
}

void LastSessionDescription::Update(base::TimeDelta duration,
                                    const std::string& id,
                                    account_util::LogoutReason logout_reason) {
  DCHECK_GT(duration, base::TimeDelta());
  DCHECK(!id.empty());
  DCHECK_NE(logout_reason, account_util::LR_UNKNOWN);
  duration_ = duration;
  id_ = id;
  logout_reason_ = logout_reason;
  Save();
}

base::TimeDelta LastSessionDescription::duration() const {
  return duration_;
}

std::string LastSessionDescription::id() const {
  return id_;
}

account_util::LogoutReason LastSessionDescription::logout_reason() const {
  return logout_reason_;
}

std::string LastSessionDescription::id_string() const {
  if (!id().empty())
    return id();
  return kNotAvailableString;
}

std::string LastSessionDescription::logout_reason_string() const {
  return account_util::LogoutReasonToString(logout_reason());
}

void LastSessionDescription::Load() {
  int64_t session_duration_int =
      pref_service_->GetInt64(kOperaSyncLastSessionDurationPref);
  duration_ = base::TimeDelta::FromInternalValue(session_duration_int);

  id_ = pref_service_->GetString(kOperaSyncLastSessionIdPref);

  int logout_reason_int =
      pref_service_->GetInteger(kOperaSyncLastSessionLogoutReasonPref);
  logout_reason_ = account_util::LR_UNKNOWN;
  if (logout_reason_int >= 0 &&
      logout_reason_int < account_util::LR_LAST_ENUM_VALUE) {
    logout_reason_ = static_cast<account_util::LogoutReason>(logout_reason_int);
  }
}

void LastSessionDescription::Save() {
  pref_service_->SetInt64(kOperaSyncLastSessionDurationPref,
                          duration_.ToInternalValue());
  pref_service_->SetString(kOperaSyncLastSessionIdPref, id_);
  int logout_reason_int = static_cast<int>(logout_reason_);
  pref_service_->SetInteger(kOperaSyncLastSessionLogoutReasonPref,
                            logout_reason_int);
}

TokenRefreshDescription::TokenRefreshDescription() {
  clear();
}

void TokenRefreshDescription::clear() {
  expired_time_ = base::Time();
  request_time_ = base::Time();
  scheduled_request_time_ = base::Time();
  request_problem_ = opera_sync::OperaAuthProblem();
  request_success_time_ = base::Time();
  request_error_time_ = base::Time();
  request_error_type_ = account_util::ADUE_NO_ERROR;
  request_auth_error_type_ = account_util::AOCE_NO_ERROR;
  request_error_message_ = std::string();
}

void TokenRefreshDescription::set_expired_time(base::Time expired_time) {
  expired_time_ = expired_time;
}

void TokenRefreshDescription::set_request_time(base::Time request_time) {
  request_time_ = request_time;
}

void TokenRefreshDescription::set_scheduled_request_time(
    base::Time scheduled_request_time) {
  scheduled_request_time_ = scheduled_request_time;
}

void TokenRefreshDescription::set_request_problem(
    opera_sync::OperaAuthProblem problem) {
  request_problem_ = problem;
}

void TokenRefreshDescription::set_success_time(base::Time success_time) {
  request_success_time_ = success_time;
}

void TokenRefreshDescription::set_error_time(base::Time error_time) {
  request_error_time_ = error_time;
}

void TokenRefreshDescription::set_error_type(
    account_util::AuthDataUpdaterError error) {
  request_error_type_ = error;
}

void TokenRefreshDescription::set_auth_error_type(
    account_util::AuthOperaComError auth_error) {
  request_auth_error_type_ = auth_error;
}

void TokenRefreshDescription::set_request_error_message(std::string message) {
  request_error_message_ = message;
}

base::Time TokenRefreshDescription::expired_time() const {
  return expired_time_;
}

base::Time TokenRefreshDescription::request_time() const {
  return request_time_;
}

base::Time TokenRefreshDescription::scheduled_request_time() const {
  return scheduled_request_time_;
}

std::string TokenRefreshDescription::request_problem_string() const {
  return request_problem_.ToString();
}

std::string TokenRefreshDescription::request_problem_token_string() const {
  return request_problem_.token();
}

std::string TokenRefreshDescription::request_problem_source_string() const {
  return opera_sync::OperaAuthProblem::SourceToString(
      request_problem_.source());
}

std::string TokenRefreshDescription::request_problem_reason_string() const {
  return opera_sync::OperaAuthProblem::ReasonToString(
      request_problem_.reason());
}

base::Time TokenRefreshDescription::success_time() const {
  return request_success_time_;
}

base::Time TokenRefreshDescription::error_time() const {
  return request_error_time_;
}

std::string TokenRefreshDescription::error_type_string() const {
  return account_util::AuthDataUpdaterErrorToString(request_error_type_);
}

std::string TokenRefreshDescription::auth_error_type_string() const {
  return account_util::AuthOperaComErrorToString(request_auth_error_type_);
}

std::string TokenRefreshDescription::request_error_message_string() const {
  return request_error_message_;
}

}  // namespace sync
}  // namespace opera
