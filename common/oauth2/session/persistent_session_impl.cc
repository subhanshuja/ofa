// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/session/persistent_session_impl.h"

#include "base/bind.h"
#include "base/guid.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/histogram_macros.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

#include "common/oauth2/pref_names.h"
#include "common/oauth2/session/session_constants.h"
#include "common/oauth2/session/session_state_observer.h"
#include "common/oauth2/util/util.h"

#if defined(OPERA_DESKTOP)
#include "desktop/common/opera_pref_names.h"
#endif  // OPERA_DESKTOP

namespace opera {
namespace oauth2 {

PersistentSessionImpl::PersistentSessionImpl(
    DiagnosticService* diagnostic_service,
    PrefService* pref_service)
    : diagnostic_service_(diagnostic_service),
      pref_service_(pref_service),
      start_method_(SSM_UNSET),
      state_(OA2SS_UNSET) {
  DCHECK(pref_service);
  if (diagnostic_service_) {
    diagnostic_service_->AddSupplier(this);
  }
}

PersistentSessionImpl::~PersistentSessionImpl() {
  if (diagnostic_service_) {
    diagnostic_service_->RemoveSupplier(this);
  }
}

// static
void PersistentSessionImpl::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  DCHECK(registry);
  registry->RegisterDictionaryPref(kOperaOAuth2SessionPref);
}

void PersistentSessionImpl::Initialize(
    base::Closure session_state_changed_callback) {
  DCHECK(!session_state_changed_callback.is_null());

  session_state_changed_callback_ = session_state_changed_callback;
}

std::string PersistentSessionImpl::GetDiagnosticName() const {
  return "session";
}

std::unique_ptr<base::DictionaryValue>
PersistentSessionImpl::GetDiagnosticSnapshot() {
  std::unique_ptr<base::DictionaryValue> snapshot(new base::DictionaryValue);
  snapshot->SetString("state", SessionStateToString(state_));
  snapshot->SetBoolean("refresh_token_present", !refresh_token_.empty());
  snapshot->SetString("username", username_);
  snapshot->SetString("id", session_id_);
  snapshot->SetDouble("start_time", start_time_.ToJsTime());
  snapshot->SetString("user_id", user_id_);
  snapshot->SetString("start_type", SessionStartMethodToString(start_method_));
  return snapshot;
}

void PersistentSessionImpl::LoadSession() {
  DCHECK(pref_service_);
  DCHECK_EQ(OA2SS_UNSET, GetState());

  const base::DictionaryValue* dict =
      pref_service_->GetDictionary(kOperaOAuth2SessionPref);
  bool session_data_empty = true;
  if (dict && !dict->empty()) {
    session_data_empty = false;
    refresh_token_ = LoadStringPref(dict, session::kRefreshToken);
    username_ = LoadStringPref(dict, session::kUserName);
    session_id_ = LoadStringPref(dict, session::kSessionId);
    user_id_ = LoadStringPref(dict, session::kUserId);

    state_ = OA2SS_UNSET;
    int64_t session_state_int = LoadInt64Pref(dict, session::kSessionState);
    if (session_state_int != -1) {
      state_ = SessionStateFromInt(session_state_int);
    }

    start_time_ = base::Time();
    int64_t session_start_time = LoadInt64Pref(dict, session::kStartTime);
    if (session_start_time != -1) {
      start_time_ = base::Time::FromInternalValue(session_start_time);
    }

    start_method_ = SSM_UNSET;
    int64_t session_start_method_int =
        LoadInt64Pref(dict, session::kStartMethod);
    if (session_start_method_int != -1) {
      start_method_ = SessionStartMethodFromInt(session_start_method_int);
    }
  }

  bool loaded_state_valid = LoadedStateValid();

  UMA_HISTOGRAM_ENUMERATION("Opera.OAuth2.Session.LoadState", state_,
                            OA2SS_LAST_ENUM_VALUE);
  enum LoadResult {
    LOAD_RESULT_EMPTY,
    LOAD_RESULT_VALID,
    LOAD_RESULT_INVALID,

    LOAD_RESULT_LAST
  };
  LoadResult result = LOAD_RESULT_EMPTY;
  if (!session_data_empty) {
    result = loaded_state_valid ? LOAD_RESULT_VALID : LOAD_RESULT_INVALID;
  }
  UMA_HISTOGRAM_ENUMERATION("Opera.OAuth2.Session.LoadResult", result,
                            LOAD_RESULT_LAST);
  ReportContentsHistogram();
  if (!loaded_state_valid) {
    VLOG(1) << "Loaded broken session, treating as empty.";
    ClearSession();
    return;
  }

  VLOG(1) << "Loaded session in state " << SessionStateToString(state_)
          << " for username " << (username_.empty() ? "<empty>" : username_);

  session_state_changed_callback_.Run();
}

void PersistentSessionImpl::StoreSession() {
  DCHECK(pref_service_);

  bool state_is_storable = StateIsStorable();
  UMA_HISTOGRAM_BOOLEAN("Opera.OAuth2.Session.IsStorable", state_is_storable);

  if (!state_is_storable) {
    VLOG(1) << "Won't store current session state.";
    return;
  }

  VLOG(1) << "Storing session with state " << SessionStateToString(state_)
          << " for username " << username_;

  DictionaryPrefUpdate updater(pref_service_, kOperaOAuth2SessionPref);
  updater.Get()->SetStringWithoutPathExpansion(
      session::kRefreshToken, crypt::OSEncrypt(refresh_token_));
  updater.Get()->SetStringWithoutPathExpansion(session::kSessionId,
                                               crypt::OSEncrypt(session_id_));
  updater.Get()->SetStringWithoutPathExpansion(session::kUserId,
                                               crypt::OSEncrypt(user_id_));
  updater.Get()->SetStringWithoutPathExpansion(session::kUserName,
                                               crypt::OSEncrypt(username_));
  updater.Get()->SetStringWithoutPathExpansion(
      session::kSessionState, crypt::OSEncryptInt64(SessionStateToInt(state_)));
  updater.Get()->SetStringWithoutPathExpansion(
      session::kStartMethod,
      crypt::OSEncryptInt64(SessionStartMethodToInt(start_method_)));
  updater.Get()->SetStringWithoutPathExpansion(
      session::kStartTime,
      crypt::OSEncryptInt64(start_time_.ToInternalValue()));
  pref_service_->CommitPendingWrite();

  RequestTakeSnapshot();
}

SessionStartMethod PersistentSessionImpl::GetStartMethod() const {
  return start_method_;
}

void PersistentSessionImpl::SetStartMethod(SessionStartMethod method) {
  start_method_ = method;
}

void PersistentSessionImpl::ClearSession() {
  refresh_token_.clear();
  username_.clear();
  session_id_.clear();
  start_time_ = base::Time();
  user_id_.clear();
  start_method_ = SSM_UNSET;
  SetState(OA2SS_INACTIVE);

  RequestTakeSnapshot();
}

SessionState PersistentSessionImpl::GetState() const {
  return state_;
}

void PersistentSessionImpl::SetState(const SessionState& state) {
  if (state == state_) {
    VLOG(2) << "Ignoring state change to " << SessionStateToString(state);
    return;
  }

  VLOG(1) << "Changing state from " << SessionStateToString(state_) << " to "
          << SessionStateToString(state);

  const auto old_state = state_;
  state_ = state;
  switch (state_) {
    case OA2SS_STARTING:
      DCHECK_EQ(old_state, OA2SS_INACTIVE);
      DCHECK_NE(SSM_UNSET, start_method_);
      DCHECK(refresh_token_.empty());
      DCHECK(!username_.empty());
      DCHECK(session_id_.empty());
      DCHECK_EQ(start_time_, base::Time());
      DCHECK(user_id_.empty());
      UpdateStartTime();
      GenerateSessionId();
      UMA_HISTOGRAM_ENUMERATION("Opera.OAuth2.Session.StartMethod",
                                start_method_, SSM_LAST_ENUM_VALUE);
      break;
    case OA2SS_IN_PROGRESS:
      DCHECK((old_state == OA2SS_STARTING) || (old_state == OA2SS_AUTH_ERROR));
      DCHECK_NE(SSM_UNSET, start_method_);
      DCHECK(!refresh_token_.empty());
      DCHECK(!username_.empty());
      DCHECK(!session_id_.empty());
      DCHECK_NE(start_time_, base::Time());
      DCHECK(!user_id_.empty());
      break;
    case OA2SS_AUTH_ERROR:
      DCHECK_NE(SSM_UNSET, start_method_);
      DCHECK(!username_.empty());
      DCHECK(!session_id_.empty());
      DCHECK_NE(start_time_, base::Time());
      user_id_.clear();
      refresh_token_.clear();
      break;
    case OA2SS_INACTIVE:
      ClearSession();
      break;
    default:
      break;
  }

  session_state_changed_callback_.Run();

  RequestTakeSnapshot();
}

void PersistentSessionImpl::SetRefreshToken(const std::string& refresh_token) {
  DCHECK(!refresh_token.empty());
  refresh_token_ = refresh_token;
}

std::string PersistentSessionImpl::GetRefreshToken() const {
  return refresh_token_;
}

std::string PersistentSessionImpl::GetUsername() const {
  return username_;
}

void PersistentSessionImpl::SetUsername(const std::string& username) {
  DCHECK(!username.empty());
  username_ = username;
}

std::string PersistentSessionImpl::GetUserId() const {
  return user_id_;
}
void PersistentSessionImpl::SetUserId(const std::string& user_id) {
  DCHECK(!user_id.empty());
  user_id_ = user_id;
}

std::string PersistentSessionImpl::GetSessionId() const {
  return session_id_;
}

base::Time PersistentSessionImpl::GetStartTime() const {
  return start_time_;
}

bool PersistentSessionImpl::HasAuthError() const {
  return state_ == OA2SS_AUTH_ERROR;
}

bool PersistentSessionImpl::IsLoggedIn() const {
  switch (state_) {
    case OA2SS_STARTING:
    case OA2SS_IN_PROGRESS:
    case OA2SS_FINISHING:
    case OA2SS_AUTH_ERROR:
      return true;
    default:
      return false;
  }
}

bool PersistentSessionImpl::IsInProgress() const {
  return state_ == OA2SS_IN_PROGRESS;
}

std::string PersistentSessionImpl::GetSessionIdForDiagnostics() const {
  DCHECK(!session_id_.empty());
  bool include_session_id_in_diagnostics = false;
#if defined(OPERA_DESKTOP)
  include_session_id_in_diagnostics =
      pref_service_->GetBoolean(prefs::kFullStatisticsEnabled);
#endif  // OPERA_DESKTOP
  if (include_session_id_in_diagnostics) {
    return session_id_;
  }
  return std::string();
}

void PersistentSessionImpl::GenerateSessionId() {
  DCHECK(session_id_.empty());
  session_id_ = base::GenerateGUID();
  DCHECK(!session_id_.empty());
}

void PersistentSessionImpl::UpdateStartTime() {
  DCHECK_EQ(OA2SS_STARTING, GetState());
  start_time_ = base::Time::Now();
}

bool PersistentSessionImpl::LoadedStateValid() const {
  switch (state_) {
    case OA2SS_INACTIVE:
      return (username_.empty() && refresh_token_.empty() &&
              session_id_.empty() && (start_time_ == base::Time()) &&
              user_id_.empty() && (start_method_ == SSM_UNSET));
    case OA2SS_IN_PROGRESS:
      return (!username_.empty() && !refresh_token_.empty() &&
              !session_id_.empty() && (start_time_ != base::Time()) &&
              !user_id_.empty() && (start_method_ != SSM_UNSET));
    case OA2SS_AUTH_ERROR:
      return (!username_.empty() && refresh_token_.empty() &&
              !session_id_.empty() && (start_time_ != base::Time()) &&
              !user_id_.empty() && (start_method_ != SSM_UNSET));
    case OA2SS_UNSET:
    case OA2SS_STARTING:
    case OA2SS_FINISHING:
    case OA2SS_LAST_ENUM_VALUE:
    default:
      return false;
  }
}

bool PersistentSessionImpl::StateIsStorable() const {
  switch (state_) {
    case OA2SS_INACTIVE:
    case OA2SS_IN_PROGRESS:
    case OA2SS_AUTH_ERROR:
      break;
    case OA2SS_UNSET:
    case OA2SS_STARTING:
    case OA2SS_FINISHING:
    case OA2SS_LAST_ENUM_VALUE:
    default:
      return false;
  }

  return true;
}

void PersistentSessionImpl::RequestTakeSnapshot() {
  if (diagnostic_service_) {
    diagnostic_service_->TakeSnapshot();
  }
}

void PersistentSessionImpl::ReportContentsHistogram() const {
  const int kHasStartMethod = 1 << 0;
  const int kHasSessionState = 1 << 1;
  const int kHasRefreshToken = 1 << 2;
  const int kHasUsername = 1 << 3;
  const int kHasSessionId = 1 << 4;
  const int kHasStartTime = 1 << 5;
  const int kHasUserId = 1 << 6;
  const int kBoundary = 1 << 7;

  int contents = 0;
  if (start_method_ != SSM_UNSET)
    contents |= kHasStartMethod;
  if (state_ != OA2SS_UNSET)
    contents |= kHasSessionState;
  if (!refresh_token_.empty())
    contents |= kHasRefreshToken;
  if (!username_.empty())
    contents |= kHasUsername;
  if (!session_id_.empty())
    contents |= kHasSessionId;
  if (start_time_ != base::Time())
    contents |= kHasStartTime;
  if (!user_id_.empty())
    contents |= kHasUserId;

  UMA_HISTOGRAM_ENUMERATION("Opera.OAuth2.Session.Contents", contents,
                            kBoundary);
}

}  // namespace oauth2
}  // namespace opera
