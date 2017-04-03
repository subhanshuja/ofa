// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/migration/oauth1_migrator_impl.h"

#include <string>
#include <utility>

#include "base/metrics/histogram_macros.h"

#include "common/oauth2/diagnostics/diagnostic_service.h"
#include "common/oauth2/network/migration_token_request.h"
#include "common/oauth2/network/migration_token_response.h"
#include "common/oauth2/network/network_request_manager.h"
#include "common/oauth2/network/oauth1_renew_token_request.h"
#include "common/oauth2/network/oauth1_renew_token_response.h"
#include "common/oauth2/util/scopes.h"
#include "common/sync/sync_config.h"
#include "common/sync/sync_login_data.h"
#include "common/sync/sync_login_data_store.h"

namespace opera {

namespace oauth2 {

namespace {

const char kDiagnosticName[] = "oauth1_migrator";

}  // namespace

OAuth1MigratorImpl::LastFetchTokenInfo::LastFetchTokenInfo()
    : oauth1_auth_error(-1),
      oauth2_auth_error(OAE_UNSET),
      time(base::Time()),
      response_status(RS_UNSET) {}

std::unique_ptr<base::DictionaryValue>
OAuth1MigratorImpl::LastFetchTokenInfo::AsDiagnosticDict() const {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  dict->SetInteger("oauth1_auth_error", oauth1_auth_error);
  dict->SetString("oauth2_auth_error",
                  OperaAuthErrorToString(oauth2_auth_error));
  dict->SetString("oauth1_error_message", oauth1_error_message);
  dict->SetDouble("result_time", time.ToJsTime());
  dict->SetString("response_status",
                  NetworkResponseStatusToString(response_status));
  return dict;
}

OAuth1MigratorImpl::OAuth1MigratorImpl(
    DiagnosticService* diagnostic_serivce,
    NetworkRequestManager* request_manager,
    DeviceNameService* device_name_service,
    std::unique_ptr<SyncLoginDataStore> oauth1_login_data_store,
    const GURL oauth1_base_url,
    const GURL oauth2_base_url,
    const std::string& oauth1_auth_service_id,
    const std::string& oauth1_client_id,
    const std::string& oauth1_client_secret,
    const std::string& oauth2_client_id)
    : request_manager_(request_manager),
      diagnostic_service_(diagnostic_serivce),
      device_name_service_(device_name_service),
      oauth1_login_data_store_(std::move(oauth1_login_data_store)),
      oauth1_base_url_(oauth1_base_url),
      oauth2_base_url_(oauth2_base_url),
      oauth1_auth_service_id_(oauth1_auth_service_id),
      oauth1_client_id_(oauth1_client_id),
      oauth1_client_secret_(oauth1_client_secret),
      oauth2_client_id_(oauth2_client_id),
      last_get_refresh_token_error_(OAE_UNSET),
      migration_result_(MR_UNSET),
      weak_ptr_factory_(this) {
  DCHECK(request_manager_);
  DCHECK(oauth1_login_data_store_);
  DCHECK(oauth1_base_url_.is_valid());
  DCHECK(oauth2_base_url_.is_valid());
  DCHECK(!oauth1_auth_service_id_.empty());
  DCHECK(!oauth1_client_id.empty());
  DCHECK(!oauth1_client_secret_.empty());
  DCHECK(!oauth2_client_id_.empty());

  oauth1_session_ = GetOAuth1SessionData();
  if (diagnostic_service_) {
    diagnostic_service_->AddSupplier(this);
  }
}

OAuth1MigratorImpl::~OAuth1MigratorImpl() {
  if (diagnostic_service_) {
    diagnostic_service_->RemoveSupplier(this);
  }
}

void OAuth1MigratorImpl::EnsureOAuth1SessionIsCleared() {
  DCHECK(oauth1_login_data_store_);
  oauth1_login_data_store_->ClearSessionAndTokenData();
  oauth1_session_.release();
}

MigrationResult OAuth1MigratorImpl::GetMigrationResult() {
  return migration_result_;
}

bool OAuth1MigratorImpl::IsMigrationPossible() {
  if (oauth1_session_)
    return true;

  return false;
}

void OAuth1MigratorImpl::PrepareMigration(PersistentSession* oauth2_session) {
  DCHECK(oauth1_session_);
  DCHECK(oauth1_session_->IsComplete());
  DCHECK(oauth2_session);
  DCHECK_EQ(OA2SS_INACTIVE, oauth2_session->GetState());

  oauth2_session_ = oauth2_session;
  oauth2_session_->SetStartMethod(SSM_OAUTH1);
  oauth2_session_->SetUsername(oauth1_session_->login);
  oauth2_session_->SetState(OA2SS_STARTING);
}

void OAuth1MigratorImpl::StartMigration() {
  DCHECK(oauth1_session_);
  DCHECK(oauth1_session_->IsComplete());
  DCHECK(oauth2_session_);
  DCHECK_EQ(OA2SS_STARTING, oauth2_session_->GetState());
  DCHECK_EQ(SSM_OAUTH1, oauth2_session_->GetStartMethod());
  DCHECK(request_manager_);

  StartOAuth2MigrationTokenRequest();
  NotifyDiagnosticStateMaybeChanged();
}

std::string OAuth1MigratorImpl::GetDiagnosticName() const {
  return kDiagnosticName;
}

std::unique_ptr<base::DictionaryValue>
OAuth1MigratorImpl::GetDiagnosticSnapshot() {
  std::unique_ptr<base::DictionaryValue> snapshot(new base::DictionaryValue);
  if (last_fetch_refresh_token_info_) {
    snapshot->Set("last_get_refresh_token_info",
                  last_fetch_refresh_token_info_->AsDiagnosticDict());
  }
  if (last_renew_oauth1_token_info_) {
    snapshot->Set("last_oauth1_response_info",
                  last_renew_oauth1_token_info_->AsDiagnosticDict());
  }
  snapshot->SetBoolean("migration_token_request_pending",
                       migration_token_request_.get() ? true : false);
  snapshot->SetBoolean("oauth1_renew_token_request_pending",
                       oauth1_renew_token_request_.get() ? true : false);
  snapshot->SetBoolean("migration_possible",
                       oauth1_session_.get() ? true : false);
  snapshot->SetString("migration_result",
                      MigrationResultToString(migration_result_));
  return snapshot;
}

void OAuth1MigratorImpl::OnNetworkRequestFinished(
    scoped_refptr<NetworkRequest> request,
    NetworkResponseStatus response_status) {
  if (request == migration_token_request_) {
    ProcessOAuth2MigrationTokenResponse(migration_token_request_,
                                        response_status);
    migration_token_request_ = nullptr;
  } else if (request == oauth1_renew_token_request_) {
    ProcessOAuth1TokenRenewalResponse(oauth1_renew_token_request_,
                                      response_status);
    oauth1_renew_token_request_ = nullptr;
  }
  NotifyDiagnosticStateMaybeChanged();
}

std::string OAuth1MigratorImpl::GetConsumerName() const {
  return kDiagnosticName;
}

void OAuth1MigratorImpl::StartOAuth1TokenRenewalRequest() {
  DCHECK(migration_token_request_);
  DCHECK(!oauth1_renew_token_request_);
  DCHECK(oauth1_session_);
  DCHECK(oauth1_session_->IsComplete());
  oauth1_renew_token_request_ = OAuth1RenewTokenRequest::Create(
      oauth1_auth_service_id_, oauth1_session_->token, oauth1_client_id_,
      oauth1_client_secret_);
  request_manager_->StartRequest(oauth1_renew_token_request_,
                                 weak_ptr_factory_.GetWeakPtr());
}

void OAuth1MigratorImpl::StartOAuth2MigrationTokenRequest() {
  DCHECK(!migration_token_request_);
  DCHECK(oauth1_session_);
  DCHECK(oauth1_session_->IsComplete());
  DCHECK(device_name_service_);
  DCHECK(oauth2_session_);

  const std::string session_id = oauth2_session_->GetSessionIdForDiagnostics();
  migration_token_request_ = MigrationTokenRequest::Create(
      oauth1_base_url_, oauth1_client_id_, oauth1_client_secret_,
      oauth1_session_->time_skew, oauth1_session_->token,
      oauth1_session_->token_secret, ScopeSet({scope::kAll}), oauth2_base_url_,
      oauth2_client_id_, device_name_service_, session_id);

  request_manager_->StartRequest(migration_token_request_,
                                 weak_ptr_factory_.GetWeakPtr());
}

void OAuth1MigratorImpl::ProcessOAuth1TokenRenewalResponse(
    scoped_refptr<OAuth1RenewTokenRequest> oauth1_token_renewal_request,
    NetworkResponseStatus response_status) {
  DCHECK(oauth1_token_renewal_request);
  DCHECK_EQ(oauth1_renew_token_request_, oauth1_token_renewal_request);

  switch (response_status) {
    case RS_OK:
      break;
    case RS_INSECURE_CONNECTION_FORBIDDEN:
      return;
    default:
      NOTREACHED();
      return;
  }

  const auto& oauth1_token_renewal_response =
      oauth1_token_renewal_request->oauth1_renew_token_response();
  DCHECK(oauth1_token_renewal_response);

  const auto oauth1_auth_error =
      oauth1_token_renewal_response->oauth1_error_code();

  last_renew_oauth1_token_info_.reset(new OAuth1TokenFetchStatus);
  last_renew_oauth1_token_info_->auth_error = oauth1_auth_error;
  last_renew_oauth1_token_info_->error_message =
      oauth1_token_renewal_response->oauth1_error_message();
  last_renew_oauth1_token_info_->response_status = response_status;

  switch (oauth1_auth_error) {
    case -1:
      DCHECK(!oauth1_token_renewal_response->oauth1_auth_token().empty());
      DCHECK(
          !oauth1_token_renewal_response->oauth1_auth_token_secret().empty());
      oauth1_session_->token =
          oauth1_token_renewal_response->oauth1_auth_token();
      oauth1_session_->token_secret =
          oauth1_token_renewal_response->oauth1_auth_token_secret();
      StartOAuth2MigrationTokenRequest();
      break;
    case 428:
      // Just in case...
      UMA_HISTOGRAM_BOOLEAN("Opera.OAuth2.Migration.BouncedWith428", true);
      StartOAuth2MigrationTokenRequest();
      break;
    default:
      VLOG(1) << "Fatal migration error " << oauth1_auth_error;
      DCHECK(oauth1_session_);
      DCHECK(oauth1_session_->IsComplete());
      oauth2_session_->SetState(OA2SS_AUTH_ERROR);
      EnsureOAuth1SessionIsCleared();
      SetMigrationResult(OAuth1ErrorToMigrationResult(oauth1_auth_error));
      break;
  }
}

void OAuth1MigratorImpl::ProcessOAuth2MigrationTokenResponse(
    scoped_refptr<MigrationTokenRequest> migration_token_request,
    NetworkResponseStatus response_status) {
  DCHECK(migration_token_request);

  switch (response_status) {
    case RS_OK:
      break;
    case RS_INSECURE_CONNECTION_FORBIDDEN:
      return;
    default:
      NOTREACHED();
      return;
  }

  const auto& migration_token_response =
      migration_token_request->migration_token_response();
  DCHECK(migration_token_response);

  const auto auth_error = migration_token_response->auth_error();
  last_fetch_refresh_token_info_.reset(new OAuth2TokenFetchStatus);
  last_fetch_refresh_token_info_->auth_error = auth_error;
  last_fetch_refresh_token_info_->error_message =
      migration_token_response->error_description();
  last_fetch_refresh_token_info_->response_status = response_status;

  switch (auth_error) {
    case OAE_OK:
      DCHECK(!migration_token_response->refresh_token().empty());
      oauth2_session_->SetRefreshToken(
          migration_token_response->refresh_token());
      oauth2_session_->SetUserId(migration_token_response->user_id());
      oauth2_session_->SetState(OA2SS_IN_PROGRESS);
      oauth2_session_->StoreSession();
      EnsureOAuth1SessionIsCleared();
      if (last_get_refresh_token_error_ == OAE_UNSET) {
        SetMigrationResult(MR_SUCCESS);
      } else {
        SetMigrationResult(MR_SUCCESS_WITH_BOUNCE);
      }

      break;
    case OAE_INVALID_GRANT:
      if (last_get_refresh_token_error_ == OAE_UNSET) {
        // Try to renew the OAuth1 token.
        StartOAuth1TokenRenewalRequest();
      } else {
        // The OAuth1 token appears useless.
        EnsureOAuth1SessionIsCleared();
        oauth2_session_->SetState(OA2SS_INACTIVE);
        SetMigrationResult(OAuth2ErrorToMigrationResult(
            migration_token_response->auth_error()));
      }
      break;
    default:
      // We have some kind of an OAuth2 error. Is this retriable? Clear the
      // old session, think about this.
      DCHECK(oauth1_session_);
      DCHECK(oauth1_session_->IsComplete());
      oauth2_session_->SetState(OA2SS_AUTH_ERROR);
      SetMigrationResult(
          OAuth2ErrorToMigrationResult(migration_token_response->auth_error()));
      EnsureOAuth1SessionIsCleared();
      break;
  }
  last_get_refresh_token_error_ = auth_error;
}

std::unique_ptr<OAuth1SessionData> OAuth1MigratorImpl::GetOAuth1SessionData()
    const {
  DCHECK(oauth1_login_data_store_);
  SyncLoginData oauth1_login_data = oauth1_login_data_store_->LoadLoginData();
  if (!oauth1_login_data.has_auth_data())
    return nullptr;
  std::unique_ptr<OAuth1SessionData> session(new OAuth1SessionData);
  session->time_skew =
      base::TimeDelta::FromSeconds(oauth1_login_data.time_skew);
  session->token = oauth1_login_data.auth_data.token;
  session->token_secret = oauth1_login_data.auth_data.token_secret;
  session->user_id = oauth1_login_data.user_id;
  if (oauth1_login_data.used_username_to_login) {
    session->login = oauth1_login_data.user_name;
  } else {
    session->login = oauth1_login_data.user_email;
  }
  session->ReportCompletnessHistogram();
  if (!session->IsComplete())
    return nullptr;

  return session;
}

MigrationResult OAuth1MigratorImpl::OAuth1ErrorToMigrationResult(
    int oauth1_error) const {
  switch (oauth1_error) {
    case 420:
      return MR_O1_420_NOT_AUTHORIZED_REQUEST;
    case 421:
      return MR_O1_421_BAD_REQUEST;
    case 422:
      return MR_O1_422_OPERA_USER_NOT_FOUND;
    case 424:
      return MR_O1_424_OPERA_TOKEN_NOT_FOUND;
    case 425:
      return MR_O1_425_INVALID_OPERA_TOKEN;
    case 426:
      return MR_O1_426_COULD_NOT_GENERATE_OPERA_TOKEN;
    case 428:
      return MR_O1_428_OPERA_ACCESS_TOKEN_NOT_EXPIRED;
    default:
      NOTREACHED();
      return MR_O1_UNKNOWN;
  }
}

MigrationResult OAuth1MigratorImpl::OAuth2ErrorToMigrationResult(
    OperaAuthError oauth2_error) const {
  switch (oauth2_error) {
    case OAE_UNSET:
      return MR_O2_UNSET;
    case OAE_INVALID_CLIENT:
      return MR_O2_INVALID_CLIENT;
    case OAE_INVALID_REQUEST:
      return MR_O2_INVALID_REQUEST;
    case OAE_INVALID_SCOPE:
      return MR_O2_INVALID_SCOPE;
    case OAE_OK:
      return MR_O2_OK;
    case OAE_INVALID_GRANT:
      return MR_O2_INVALID_GRANT;
    default:
      NOTREACHED();
      return MR_O2_UNKNOWN;
  }
}

void OAuth1MigratorImpl::SetMigrationResult(MigrationResult result) {
  UMA_HISTOGRAM_ENUMERATION("Opera.OAuth2.Migration.Result", result,
                            MR_LAST_ENUM_VALUE);
  migration_result_ = result;
}

void OAuth1MigratorImpl::NotifyDiagnosticStateMaybeChanged() {
  if (diagnostic_service_) {
    diagnostic_service_->TakeSnapshot();
  }
}

}  // namespace oauth2

}  // namespace opera
