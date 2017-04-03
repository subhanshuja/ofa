// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/auth_service_impl.h"

#include <string>
#include <utility>

#include "base/guid.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"

#include "common/oauth2/client/auth_service_client.h"
#include "common/oauth2/migration/oauth1_migrator_impl.h"
#include "common/oauth2/network/access_token_request.h"
#include "common/oauth2/network/access_token_response.h"
#include "common/oauth2/network/network_request.h"
#include "common/oauth2/network/network_request_manager_impl.h"
#include "common/oauth2/network/request_throttler.h"
#include "common/oauth2/network/revoke_token_request.h"
#include "common/oauth2/network/revoke_token_response.h"
#include "common/oauth2/network/request_vars_encoder_impl.h"
#include "common/oauth2/session/session_state_observer.h"
#include "common/oauth2/token_cache/token_cache_impl.h"
#include "common/oauth2/util/constants.h"
#include "common/oauth2/util/scopes.h"
#include "common/oauth2/util/util.h"

namespace opera {
namespace oauth2 {

namespace {
const char kAuthServiceName[] = "auth_service";
}

AuthServiceImpl::AuthServiceImpl()
    : diagnostic_service_(nullptr),
      last_session_end_reason_(SER_UNKNOWN),
      device_name_service_(nullptr),
      token_cache_loaded_(false),
      weak_ptr_factory_(this) {
  VLOG(4) << "Auth service created.";
}

AuthServiceImpl::~AuthServiceImpl() {
  VLOG(4) << "Auth service destroyed.";
}

void AuthServiceImpl::Initialize(std::unique_ptr<InitParams> params) {
  VLOG(4) << "AuthService initializing";
  DCHECK(params);
  DCHECK(params->IsValid());
  params_ = std::move(params);
  task_runner_ = params_->task_runner;
  diagnostic_service_ = params_->diagnostic_service;
  if (diagnostic_service_) {
    diagnostic_service_->AddSupplier(this);
  }

  session_ = std::move(params_->oauth2_session);
  session_->Initialize(base::Bind(&AuthServiceImpl::SessionStateChangedCallback,
                                  weak_ptr_factory_.GetWeakPtr()));
  session_->LoadSession();

  oauth1_migrator_ = std::move(params_->oauth1_migrator);
  DCHECK(oauth1_migrator_);

  bool session_is_inactive = session_->GetState() == OA2SS_INACTIVE;
  bool migration_is_possible = oauth1_migrator_->IsMigrationPossible();

  if (session_is_inactive) {
    UMA_HISTOGRAM_BOOLEAN("Opera.OAuth2.Migration.WasPrepared",
                          migration_is_possible);
    if (migration_is_possible) {
      VLOG(1) << "Auth service will perform migration.";
      oauth1_migrator_->PrepareMigration(session_.get());
    }
  }

  token_cache_ = params_->oauth2_token_cache;
  token_cache_->Load(base::Bind(&AuthServiceImpl::OnTokenCacheLoaded,
                                weak_ptr_factory_.GetWeakPtr()));

  NotifyDiagnosticStateMaybeChanged();
}

void AuthServiceImpl::OnTokenCacheLoaded() {
  DCHECK(session_);
  DCHECK(!token_cache_loaded_);

  token_cache_loaded_ = true;

  request_manager_ = std::move(params_->oauth2_network_request_manager);
  device_name_service_ = params_->device_name_service;

  DCHECK(oauth1_migrator_);
  DCHECK(request_manager_);
  DCHECK(device_name_service_);

  VLOG(4) << "Auth service: token cache loaded.";
  request_throttler_.reset(new RequestThrottler(params_->backoff_clock));

  switch (session_->GetState()) {
    case OA2SS_INACTIVE:
      oauth1_migrator_.reset();
      break;
    case OA2SS_STARTING:
      DCHECK(oauth1_migrator_);
      DCHECK(oauth1_migrator_->IsMigrationPossible());
      VLOG(1) << "Auth service starting migration.";
      oauth1_migrator_->StartMigration();
      break;
    case OA2SS_IN_PROGRESS:
      ProcessRequestTokenCallbacks();
    // Fall through.
    case OA2SS_AUTH_ERROR:
      if (oauth1_migrator_) {
        oauth1_migrator_->EnsureOAuth1SessionIsCleared();
        oauth1_migrator_.reset();
      }
      break;
    default:
      NOTREACHED();
  }

  DCHECK(request_token_callbacks_.empty());
  SessionStateChangedCallback();
}

void AuthServiceImpl::SessionStateChangedCallback() {
  DCHECK(session_);
  if (!token_cache_loaded_) {
    return;
  }

  FOR_EACH_OBSERVER(SessionStateObserver, session_state_observers_,
                    OnSessionStateChanged());
  NotifyDiagnosticStateMaybeChanged();
}

void AuthServiceImpl::Shutdown() {
  VLOG(4) << "AuthService shutting down";

  DCHECK(session_);
  session_->StoreSession();

  DCHECK(token_cache_);
  token_cache_->Store();

  if (diagnostic_service_) {
    diagnostic_service_->RemoveSupplier(this);
  }
}

std::string AuthServiceImpl::GetDiagnosticName() const {
  return kAuthServiceName;
}

std::unique_ptr<base::DictionaryValue>
AuthServiceImpl::GetDiagnosticSnapshot() {
  std::unique_ptr<base::DictionaryValue> snapshot(new base::DictionaryValue);
  snapshot->SetBoolean("refresh_token_request_pending",
                       refresh_token_for_sso_request_ ? true : false);
  snapshot->SetInteger("pending_access_token_requests",
                       running_access_token_requests_.size());

  std::unique_ptr<base::ListValue> clients(new base::ListValue);
  for (const auto& client_registration : client_registrations_) {
    clients->Append(
        base::MakeUnique<base::StringValue>(client_registration.second));
  }
  snapshot->Set("clients", std::move(clients));
  if (last_fetch_refresh_token_info_) {
    snapshot->Set("last_fetch_refresh_token_info",
                  last_fetch_refresh_token_info_->AsDiagnosticDict());
  }
  if (last_fetch_access_token_info_) {
    snapshot->Set("last_fetch_access_token_info",
                  last_fetch_access_token_info_->AsDiagnosticDict());
  }
  snapshot->SetString("last_session_end_reason",
                      SessionEndReasonToString(last_session_end_reason_));

  return snapshot;
}

std::string AuthServiceImpl::GetConsumerName() const {
  return kAuthServiceName;
}

void AuthServiceImpl::OnNetworkRequestFinished(
    scoped_refptr<NetworkRequest> request,
    NetworkResponseStatus response_status) {
  DCHECK(token_cache_loaded_);
  DCHECK(session_);
  DCHECK(request);
  if (request == refresh_token_for_sso_request_) {
    VLOG(2) << "Refresh token request completed.";
    ProcessRefreshTokenRequest(refresh_token_for_sso_request_, response_status);
    refresh_token_for_sso_request_ = nullptr;
  } else {
    for (auto running_request : running_access_token_requests_) {
      if (running_request.first == request) {
        const auto client = running_request.second;
        if (IsClientKnown(client)) {
          VLOG(2) << "Access token request completed for client '"
                  << GetClientName(client) << "'.";
          ProcessAccessTokenRequest(running_request.first, client,
                                    response_status);
        } else {
          VLOG(2) << "Got an access token network response for client that is "
                     "already gone.";
        }
        running_access_token_requests_.erase(running_request.first);
        break;
      }
    }
  }

  VLOG(4) << "Network request to " << request->GetPath() << " completed.";
  NotifyDiagnosticStateMaybeChanged();
}

void AuthServiceImpl::StartSessionWithAuthToken(const std::string& username,
                                                const std::string& auth_token) {
  DCHECK(token_cache_loaded_);
  DCHECK(session_);
  const auto session_state = session_->GetState();
  DCHECK(session_state == OA2SS_INACTIVE || session_state == OA2SS_AUTH_ERROR)
      << session_state;
  DCHECK(!username.empty());
  DCHECK(!auth_token.empty());
  DCHECK(request_manager_);

  VLOG(1) << "Session starting for '" << username << "' with auth token.";

  if (session_->HasAuthError() && username != session_->GetUsername()) {
    // NOTE: When logging in again via the sync popup with the auth.opera.com
    // login form user can enter any credentials. In case the session is in
    // the AUTH_ERROR state, the newly entered credentials may differ from the
    // credentials used the last time. The new and old username will differ.
    // In such a case we will terminate the session immediatelly. This is a
    // unlikely corner case, but still we should perhaps take this into the
    // account in case the UI gets a touch. Or pehaps this can be approached
    // on auth.opera.com so that the login form disallows editing the username
    // if requested.
    VLOG(1) << "Logged in as different user, terminating session.";
    EndSession(SER_USERNAME_CHANGED_DURING_RELOGIN);
    return;
  }

  // Reset state in case this is a new login from the AUTH_ERROR state.
  session_->SetState(OA2SS_INACTIVE);
  session_->SetStartMethod(SSM_AUTH_TOKEN);
  session_->SetUsername(username);
  session_->SetState(OA2SS_STARTING);

  DCHECK(!session_->GetSessionId().empty());
  const std::string session_id = session_->GetSessionIdForDiagnostics();

  refresh_token_for_sso_request_ = AccessTokenRequest::CreateWithAuthTokenGrant(
      auth_token, ScopeSet({scope::kAll}), params_->client_id,
      device_name_service_, session_id);
  request_manager_->StartRequest(refresh_token_for_sso_request_,
                                 weak_ptr_factory_.GetWeakPtr());

  NotifyDiagnosticStateMaybeChanged();
}

void AuthServiceImpl::EndSession(SessionEndReason reason) {
  DCHECK(token_cache_loaded_);
  DCHECK(session_);
  VLOG(1) << "Session end request with reason " << SessionEndReason(reason);
  DCHECK(token_cache_);
  DCHECK(session_);
  const auto session_state = session_->GetState();
  DCHECK(session_state == OA2SS_IN_PROGRESS ||
         session_state == OA2SS_AUTH_ERROR || session_state == OA2SS_STARTING);

  UMA_HISTOGRAM_ENUMERATION("Opera.OAuth2.Session.EndReason", reason,
                            SER_LAST_ENUM_VALUE);

  if (oauth1_migrator_) {
    oauth1_migrator_->EnsureOAuth1SessionIsCleared();
    oauth1_migrator_.reset();
  }

  last_session_end_reason_ = reason;

  device_name_service_->ClearLastDeviceName();

  running_access_token_requests_.clear();
  pending_requests_.clear();
  refresh_token_for_sso_request_ = nullptr;
  token_cache_->Clear();
  request_manager_->CancelAllRequests();
  request_throttler_.reset(new RequestThrottler(params_->backoff_clock));

  if (session_->GetState() == OA2SS_IN_PROGRESS) {
    DCHECK(!session_->GetRefreshToken().empty());
    RevokeRefreshToken(session_->GetRefreshToken());
    session_->SetState(opera::oauth2::OA2SS_FINISHING);
  } else {
    DCHECK(session_->GetRefreshToken().empty());
  }
  session_->ClearSession();
  session_->StoreSession();
  NotifyDiagnosticStateMaybeChanged();
}

void AuthServiceImpl::RevokeRefreshToken(const std::string& refresh_token) {
  DCHECK(token_cache_loaded_);
  DCHECK(session_);
  DCHECK(!session_->GetSessionId().empty());

  VLOG(1) << "Auth service revoking refresh token.";

  const std::string session_id = session_->GetSessionIdForDiagnostics();
  auto request =
      RevokeTokenRequest::Create(RevokeTokenRequest::TT_REFRESH_TOKEN,
                                 refresh_token, params_->client_id, session_id);
  request_manager_->StartRequest(request, weak_ptr_factory_.GetWeakPtr());
}

void AuthServiceImpl::RequestAccessToken(AuthServiceClient* client,
                                         ScopeSet scopes) {
  DCHECK(session_);
  DCHECK(client);
  DCHECK(IsClientKnown(client));
  // RFC 6749 allows multiple scopes for a token request, we don't need that
  // ATM however.
  DCHECK_EQ(scopes.size(), 1u);
  DCHECK(token_cache_);

  const std::string& client_name = GetClientName(client);
  VLOG(1) << "Client '" << client_name
          << "' requesting access token with scope '" << scopes.encode()
          << "'.";

  if (session_->GetState() != OA2SS_IN_PROGRESS) {
    VLOG(1) << "Current session state is "
            << SessionStateToString(session_->GetState())
            << ", ignoring access token request.";
    client->OnAccessTokenRequestDenied(this, scopes);
    return;
  }

  // The request key is supposed to uniquely identify a request for a specific
  // access token. It's used both for throttling the request rate and caching
  // the access tokens.
  const std::string& request_key = client_name + scopes.encode();
  if (pending_requests_.count(request_key)) {
    VLOG(1) << "Request already pending, ignoring.";
    return;
  }

  pending_requests_.insert(request_key);

  base::Closure request_token_callback =
      base::Bind(&AuthServiceImpl::DoRequestAccessToken,
                 weak_ptr_factory_.GetWeakPtr(), client, scopes);

  if (!token_cache_loaded_) {
    VLOG(1) << "Access token request delayed until token cache loaded.";
    request_token_callbacks_.push_back(request_token_callback);
  } else {
    DCHECK(device_name_service_);
    DCHECK(request_manager_.get());
    DCHECK(request_throttler_.get());

    base::TimeDelta request_delay =
        request_throttler_->GetAndUpdateRequestDelay(request_key);
    VLOG(1) << "Token request scheduled with delay " << request_delay;
    task_runner_->PostDelayedTask(FROM_HERE, request_token_callback,
                                  request_delay);
  }
}

void AuthServiceImpl::SignalAccessTokenProblem(
    AuthServiceClient* client,
    scoped_refptr<const AuthToken> token) {
  DCHECK(token_cache_loaded_);
  DCHECK(session_);
  DCHECK(IsClientKnown(client));

  VLOG(1) << "Client " << GetClientName(client) << " signals token problem.";
  token_cache_->EvictToken(token);
}

void AuthServiceImpl::RegisterClient(AuthServiceClient* client,
                                     const std::string& client_name) {
  DCHECK(client);
  DCHECK(!client_name.empty());
  DCHECK(!IsClientKnown(client)) << "Client registered twice";

  VLOG(4) << "Registering client '" << client_name << "'";
  client_registrations_[client] = client_name;
  NotifyDiagnosticStateMaybeChanged();
}

void AuthServiceImpl::UnregisterClient(AuthServiceClient* client) {
  DCHECK(client);
  DCHECK(IsClientKnown(client)) << "Client not registered.";

  const auto& client_name = client_registrations_[client];
  VLOG(4) << "Unregistering client '" << client_name << "'";
  client_registrations_.erase(client);
  NotifyDiagnosticStateMaybeChanged();
}

void AuthServiceImpl::AddSessionStateObserver(SessionStateObserver* observer) {
  DCHECK(observer);
  DCHECK(!session_state_observers_.HasObserver(observer));
  session_state_observers_.AddObserver(observer);
}

void AuthServiceImpl::RemoveSessionStateObserver(
    SessionStateObserver* observer) {
  DCHECK(observer);
  DCHECK(session_state_observers_.HasObserver(observer));
  session_state_observers_.RemoveObserver(observer);
}

const PersistentSession* AuthServiceImpl::GetSession() const {
  return session_.get();
}

void AuthServiceImpl::DoRequestAccessToken(AuthServiceClient* client,
                                           ScopeSet scopes) {
  DCHECK(client);
  DCHECK(token_cache_loaded_);
  if (!IsClientKnown(client)) {
    VLOG(1)
        << "Auth service: client gone while deferring access token request.";
    return;
  }

  DCHECK(session_);
  DCHECK(client);
  DCHECK(!scopes.empty());

  VLOG(1) << "Auth service processing deferred access token request for "
          << GetClientName(client);

  if (session_->GetState() != OA2SS_IN_PROGRESS) {
    VLOG(1) << "Current session state is "
            << SessionStateToString(session_->GetState())
            << ", dropping deferred access token request.";
    client->OnAccessTokenRequestDenied(this, scopes);
    return;
  }

  const std::string& request_key = GetClientName(client) + scopes.encode();
  if (pending_requests_.count(request_key) == 0) {
    VLOG(1) << "A throttled token request was cancelled along the way.";
    return;
  }

  pending_requests_.erase(request_key);
  auto cached_token = token_cache_->GetToken(GetClientName(client), scopes);
  if (cached_token) {
    // TODO(mzajaczkowski): Avoid returning the response on same callstack.
    VLOG(1) << "Token found in cache, notyfying client.";
    client->OnAccessTokenRequestCompleted(
        this, RequestAccessTokenResponse{OAE_OK, scopes, cached_token});
  } else {
    VLOG(1) << "Token not found in cache.";
    bool request_already_running = false;

    for (const auto& running_request : running_access_token_requests_) {
      AuthServiceClient* requesting_client = running_request.second;
      ScopeSet requested_scopes = running_request.first->requested_scopes();
      if (requesting_client == client &&
          requested_scopes.encode() == scopes.encode()) {
        request_already_running = true;
        break;
      }
    }

    if (request_already_running) {
      VLOG(1) << "Access token request for client '" << GetClientName(client)
              << "' and scope set '" << scopes.encode() << "' already queued.";
    } else {
      VLOG(1) << "Starting access token request for client '"
              << GetClientName(client) << "' and scope set '" << scopes.encode()
              << "'.";
      const std::string refresh_token = session_->GetRefreshToken();
      DCHECK(!refresh_token.empty());
      const std::string session_id = session_->GetSessionIdForDiagnostics();
      scoped_refptr<AccessTokenRequest> access_token_request =
          AccessTokenRequest::CreateWithRefreshTokenGrant(
              refresh_token, scopes, params_->client_id, device_name_service_,
              session_id);
      request_manager_->StartRequest(access_token_request,
                                     weak_ptr_factory_.GetWeakPtr());
      running_access_token_requests_[access_token_request] = client;
    }
  }
  NotifyDiagnosticStateMaybeChanged();
}

void AuthServiceImpl::ProcessRefreshTokenRequest(
    scoped_refptr<AccessTokenRequest> request,
    NetworkResponseStatus response_status) {
  DCHECK(token_cache_loaded_);
  DCHECK(session_);

  last_fetch_refresh_token_info_.reset(new OAuth2TokenFetchStatus);
  last_fetch_refresh_token_info_->origin =
      SessionStartMethodToString(session_->GetStartMethod());
  last_fetch_refresh_token_info_->response_status = response_status;

  switch (response_status) {
    case RS_OK:
      break;
    case RS_INSECURE_CONNECTION_FORBIDDEN:
      // Don't treat this as an auth error, don't break the session
      // due to what appears to be a command line misconfiguration.
      NotifyDiagnosticStateMaybeChanged();
      return;
    default:
      NOTREACHED();
      NotifyDiagnosticStateMaybeChanged();
      return;
  }

  AccessTokenResponse* response = request->access_token_response();
  DCHECK(response);
  const auto auth_error = response->auth_error();
  last_fetch_refresh_token_info_->auth_error = auth_error;
  last_fetch_refresh_token_info_->error_message = response->error_decription();

  VLOG(1) << "Auth service got refresh token response with result '"
          << OperaAuthErrorToString(auth_error) << "'.";

  UMA_HISTOGRAM_ENUMERATION("Opera.OAuth2.RefreshTokenRequest.Result",
                            auth_error, OAE_LAST_ENUM_VALUE);

  switch (auth_error) {
    case OAE_OK:
      DCHECK(!response->refresh_token().empty());
      session_->SetRefreshToken(response->refresh_token());
      session_->SetUserId(response->user_id());
      session_->SetState(OA2SS_IN_PROGRESS);
      session_->StoreSession();
      break;
    default:
      EnterAuthError(AER_REFRESH_TOKEN, auth_error);
      break;
  }
  NotifyDiagnosticStateMaybeChanged();
}

void AuthServiceImpl::ProcessAccessTokenRequest(
    scoped_refptr<AccessTokenRequest> request,
    AuthServiceClient* requesting_client,
    NetworkResponseStatus response_status) {
  DCHECK(token_cache_loaded_);
  DCHECK(session_);
  DCHECK(request);
  DCHECK(requesting_client);
  DCHECK(IsClientKnown(requesting_client));
  ScopeSet requested_scopes = request->requested_scopes();
  DCHECK(!requested_scopes.empty());

  const auto client_name = GetClientName(requesting_client);
  last_fetch_access_token_info_.reset(new OAuth2TokenFetchStatus);
  last_fetch_access_token_info_->origin = client_name;
  last_fetch_access_token_info_->response_status = response_status;

  switch (response_status) {
    case RS_OK:
      break;
    case RS_INSECURE_CONNECTION_FORBIDDEN:
      // Don't treat this as an auth error, don't break the session
      // due to what appears to be a command line misconfiguration.
      NotifyDiagnosticStateMaybeChanged();
      return;
    default:
      NOTREACHED();
      return;
  }

  DCHECK(request->access_token_response());
  AccessTokenResponse* access_token_response = request->access_token_response();
  ScopeSet granted_scopes = requested_scopes;
  scoped_refptr<AuthToken> token;
  const auto auth_error = access_token_response->auth_error();

  UMA_HISTOGRAM_ENUMERATION("Opera.OAuth2.AccessTokenRequest.Result",
                            auth_error, OAE_LAST_ENUM_VALUE);

  last_fetch_access_token_info_->auth_error = auth_error;
  last_fetch_access_token_info_->error_message =
      access_token_response->error_decription();

  VLOG(1) << "Got an access token response for client '"
          << GetClientName(requesting_client) << "' with result '"
          << OperaAuthErrorToString(auth_error) << "'.";

  bool response_was_auth_error = false;
  switch (auth_error) {
    case OAE_OK:
      if (!access_token_response->scopes().empty()) {
        granted_scopes = access_token_response->scopes();
      }

      token = new AuthToken(
          client_name, access_token_response->access_token(), granted_scopes,
          base::Time::Now() + access_token_response->expires_in());

      VLOG(3) << "Got access token: " << token->GetDiagnosticDescription();

      token_cache_->PutToken(token);
      break;
    case OAE_INVALID_REQUEST:
    case OAE_INVALID_SCOPE:
    case OAE_INVALID_CLIENT:
    case OAE_INVALID_GRANT:
      response_was_auth_error = true;
      break;
    default:
      VLOG(1) << "Unknown auth response " << auth_error;
      response_was_auth_error = true;
      break;
  }

  if (response_was_auth_error) {
    VLOG(1) << "Access token request failed with auth error.";
    EnterAuthError(AER_ACCESS_TOKEN, auth_error);
  }

  requesting_client->OnAccessTokenRequestCompleted(
      this, RequestAccessTokenResponse{auth_error, requested_scopes, token});
}

std::string AuthServiceImpl::GetClientName(AuthServiceClient* client) const {
  DCHECK_EQ(client_registrations_.count(client), 1u);
  DCHECK(!client_registrations_.at(client).empty());
  return client_registrations_.at(client);
}

bool AuthServiceImpl::IsClientKnown(AuthServiceClient* client) const {
  DCHECK(client);
  return (client_registrations_.count(client) == 1);
}

void AuthServiceImpl::NotifyDiagnosticStateMaybeChanged() {
  if (diagnostic_service_) {
    diagnostic_service_->TakeSnapshot();
  }
}

void AuthServiceImpl::ProcessRequestTokenCallbacks() {
  VLOG(1) << "Running " << request_token_callbacks_.size()
          << " delayed token requests.";
  for (auto callback : request_token_callbacks_) {
    DCHECK(!callback.is_null());
    callback.Run();
  }

  request_token_callbacks_.clear();
}

void AuthServiceImpl::EnterAuthError(AuthErrorReason reason,
                                     OperaAuthError auth_error) {
  const auto refresh_token = session_->GetRefreshToken();
  session_->SetState(OA2SS_AUTH_ERROR);
  session_->StoreSession();
  running_access_token_requests_.clear();
  pending_requests_.clear();
  token_cache_->Clear();
  request_manager_->CancelAllRequests();
  if (reason == AER_ACCESS_TOKEN) {
    DCHECK(!refresh_token.empty());
    RevokeRefreshToken(refresh_token);
  }
  NotifyDiagnosticStateMaybeChanged();
}

}  // namespace oauth2
}  // namespace opera
