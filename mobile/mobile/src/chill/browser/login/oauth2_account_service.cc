// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2017 Opera Software AS.  All rights reserved.
//
// This file is an original work developed by Opera Software.

#include "chill/browser/login/oauth2_account_service.h"

#include <utility>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chill/browser/login/oauth2_account_service_observer.h"
#include "common/auth/auth_token_fetcher.h"
#include "common/oauth2/auth_service.h"
#include "common/oauth2/session/persistent_session.h"
#include "common/oauth2/util/util.h"

namespace opera {
namespace {

OAuth2AccountError AuthServiceErrorToAccountError(AuthServiceError error) {
  switch (error) {
    case CONNECTION_FAILED:
      return ERR_NETWORK_ERROR;
    case INVALID_CREDENTIALS:
      return ERR_INVALID_CREDENTIALS;
    case INTERNAL_ERROR:
    case UNEXPECTED_RESPONSE:
      return ERR_SERVICE_ERROR;
    case NONE:
    default:
      return ERR_NONE;
  }
}

}  // namespace

OAuth2AccountService::OAuth2AccountService(oauth2::AuthService* auth_service,
                                           net::URLRequestContextGetter* getter)
    : auth_service_(auth_service),
      request_context_getter_(getter),
      weak_ptr_factory_(this) {
  was_logged_in_ = IsLoggedIn();
  auth_service_->AddSessionStateObserver(this);
}

OAuth2AccountService::~OAuth2AccountService() {
  FOR_EACH_OBSERVER(OAuth2AccountServiceObserver, service_observers_,
                    OnServiceDestroyed());
}

void OAuth2AccountService::StartSessionWithCredentials(
    const std::string& username,
    const std::string& password,
    const SessionStartCallback& callback) {
  pending_session_start_callback_ = callback;
  if (IsLoggedIn()) {
    EndSession();
  }

  token_fetcher_ = base::MakeUnique<AuthTokenFetcher>(request_context_getter_);
  token_fetcher_->RequestAuthTokenForOAuth2(username, password,
                                            weak_ptr_factory_.GetWeakPtr());
}

void OAuth2AccountService::StartSessionWithAuthToken(
    const std::string& username,
    const std::string& token,
    const SessionStartCallback& callback) {
  pending_session_start_callback_ = callback;
  if (IsLoggedIn()) {
    EndSession();
  }

  auth_service_->StartSessionWithAuthToken(username, token);
}

void OAuth2AccountService::EndSession() {
  token_fetcher_.reset();
  auth_service_->EndSession(oauth2::SER_USER_REQUESTED_LOGOUT);
}

bool OAuth2AccountService::IsLoggedIn() const {
  // "In progress" means the session is active and has no error, meaning we
  // impose a stricter definition of what constitutes "logged in" compared to
  // oauth2::AuthService
  return auth_service_->GetSession()->IsInProgress();
}

void OAuth2AccountService::AddObserver(OAuth2AccountServiceObserver* observer) {
  service_observers_.AddObserver(observer);
}

void OAuth2AccountService::RemoveObserver(
    OAuth2AccountServiceObserver* observer) {
  service_observers_.RemoveObserver(observer);
}

void OAuth2AccountService::OnAuthSuccess(const std::string& auth_token,
                                         std::unique_ptr<UserData> user_data) {
  auth_service_->StartSessionWithAuthToken(
      user_data->username.empty() ? user_data->email : user_data->username,
      auth_token);
  DeleteAuthTokenFetcherSoon();
}

void OAuth2AccountService::OnAuthError(AuthServiceError auth_error,
                                       const std::string& error_msg) {
  CallPendingStartCallback(
      {.error = AuthServiceErrorToAccountError(auth_error)});
  DeleteAuthTokenFetcherSoon();
}

void OAuth2AccountService::OnSessionStateChanged() {
  const oauth2::PersistentSession* session = auth_service_->GetSession();
  oauth2::SessionState new_state = session->GetState();

  if (!pending_session_start_callback_.is_null()) {
    if (new_state == oauth2::OA2SS_IN_PROGRESS) {
      CallPendingStartCallback(
          {.error = ERR_NONE, .username = session->GetUsername()});
    } else if (new_state == oauth2::OA2SS_AUTH_ERROR) {
      CallPendingStartCallback({.error = ERR_SERVICE_ERROR});
      pending_session_start_callback_ = SessionStartCallback();
    }
  }

  if (was_logged_in_ && !IsLoggedIn()) {
    was_logged_in_ = false;
    NotifyLoggedOut();
  }
}

void OAuth2AccountService::DeleteAuthTokenFetcherSoon() {
  // Wait for AuthTokenFetcher to settle down before deleting it
  if (token_fetcher_) {
    base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE,
                                                    token_fetcher_.release());
  }
}

void OAuth2AccountService::CallPendingStartCallback(
    const SessionStartResult& result) {
  // TODO(ingemara): Remove this method once we get OnceCallback
  pending_session_start_callback_.Run(result);
  pending_session_start_callback_ = SessionStartCallback();
}

void OAuth2AccountService::NotifyLoggedOut() {
  FOR_EACH_OBSERVER(OAuth2AccountServiceObserver, service_observers_,
                    OnLoggedOut());
}

}  // namespace opera
