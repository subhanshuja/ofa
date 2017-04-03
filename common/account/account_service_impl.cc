// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/account/account_service_impl.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_number_conversions.h"
#include "google_apis/gaia/oauth_request_signer.h"
#include "net/base/net_errors.h"

#include "common/account/account_observer.h"
#include "common/account/account_service_delegate.h"
#include "common/sync/sync_config.h"

namespace opera {
AccountService* AccountService::Create(const std::string& client_key,
                                       const std::string& client_secret,
                                       AccountServiceDelegate* delegate) {
  return new AccountServiceImpl(client_key, client_secret, delegate);
}

AccountServiceImpl::AccountServiceImpl(const std::string& client_key,
                                       const std::string& client_secret,
                                       AccountServiceDelegate* delegate)
    : client_key_(client_key),
      client_secret_(client_secret),
      time_skew_(0),
      delegate_(delegate),
      retry_timeout_(kDefaultRetryTimeout) {
  last_token_request_time_ = base::Time();
  next_token_request_time_ = base::Time();
}

AccountServiceImpl::~AccountServiceImpl() {
  DCHECK(CalledOnValidThread());
}

void AccountServiceImpl::AddObserver(AccountObserver* observer) {
  DCHECK(CalledOnValidThread());
  observers_.AddObserver(observer);
}

void AccountServiceImpl::RemoveObserver(AccountObserver* observer) {
  DCHECK(CalledOnValidThread());
  observers_.RemoveObserver(observer);
}

void AccountServiceImpl::LogIn() {
  VLOG(1) << "Authorization requested";
  opera_sync::OperaAuthProblem problem;
  problem.set_token("");
  problem.set_source(opera_sync::OperaAuthProblem::SOURCE_LOGIN);
  LogIn(problem);
}

void AccountServiceImpl::LogIn(opera_sync::OperaAuthProblem problem) {
  DCHECK(CalledOnValidThread());

  VLOG(1) << "Authorization requested with problem " << problem.ToString();

  // Make sure this is called before OnLoggedIn() in case we already
  // do have valid auth data.
  FOR_EACH_OBSERVER(AccountObserver, observers_,
                    OnLoginRequested(this, problem));

  if (NotifyIfLoggedIn(problem))
    return;

  if (opera::SyncConfig::ShouldUseAuthTokenRecovery()) {
    if (request_token_timer_.IsRunning()) {
      VLOG(1) << "Authorization request [" << problem.ToString()
              << "] ignored since another request is already pending.";
      FOR_EACH_OBSERVER(AccountObserver, observers_,
                        OnLoginRequestIgnored(this, problem));
      return;
    }
    if (ShouldDelayTokenRequest()) {
      DCHECK_NE(DeltaTillNextTokenRequestAllowed(), base::TimeDelta());
      next_token_request_time_ =
          base::Time::Now() + DeltaTillNextTokenRequestAllowed();
      VLOG(1) << "Auth data request scheduled to be done at "
              << next_token_request_time_;

      request_token_timer_.Start(
          FROM_HERE, DeltaTillNextTokenRequestAllowed(),
          base::Bind(&AccountService::DoRequestAuthData, AsWeakPtr(), problem));
      FOR_EACH_OBSERVER(
          AccountObserver, observers_,
          OnLoginRequestDeferred(this, problem, next_token_request_time_));
    } else {
      VLOG(1) << "Auth data request sent immediately.";
      DoRequestAuthData(problem);
    }
  } else {
    // The old way, request the new auth data immediatelly.
    DoRequestAuthData(problem);
  }
}

void AccountServiceImpl::LogOut(account_util::LogoutReason logout_reason) {
  DCHECK(CalledOnValidThread());
  VLOG(1) << "Logout requested";

  FOR_EACH_OBSERVER(AccountObserver, observers_,
                    OnLogoutRequested(this, logout_reason));

  auth_data_ = AccountAuthData();
  delegate_->LoggedOut();

  FOR_EACH_OBSERVER(AccountObserver, observers_,
                    OnLoggedOut(this, logout_reason));
}

void AccountServiceImpl::AuthDataExpired(opera_sync::OperaAuthProblem problem) {
  VLOG(1) << "Authorization data expired";
  DCHECK(CalledOnValidThread());
  auth_data_ = AccountAuthData();
  delegate_->InvalidateAuthData();

  FOR_EACH_OBSERVER(AccountObserver, observers_,
                    OnAuthDataExpired(this, problem));
}

std::string AccountServiceImpl::GetSignedAuthHeader(
    const GURL& request_base_url,
    HttpMethod http_method,
    const std::string& realm) const {
  DCHECK(CalledOnValidThread());
  return GetSignedAuthHeaderWithTimestamp(request_base_url, http_method, realm,
                                          "", "");
}

void AccountServiceImpl::SetTimeSkew(int64_t time_skew) {
  time_skew_ = time_skew;
}

std::string AccountServiceImpl::GetSignedAuthHeaderWithTimestamp(
    const GURL& request_base_url,
    HttpMethod http_method,
    const std::string& realm,
    const std::string& timestamp,
    const std::string& nonce) const {
  DCHECK(CalledOnValidThread());
  if (!HasAuthToken()) {
    // Note that it is possible that the auth token will not be
    // available at this moment, i.e. the XMPP service might request
    // a token for reconnection just in-between token refresh request
    // was sent and served. Sample log to be found in DNA-48030.
    VLOG(1) << "Auth token is not available now.";
    return std::string();
  }

  OAuthRequestSigner::Parameters parameters;
  parameters["opera_time_skew"] = base::Int64ToString(time_skew_);
  if (!timestamp.empty())
    parameters["oauth_timestamp"] = timestamp;
  if (!nonce.empty())
    parameters["oauth_nonce"] = nonce;

  std::string result;
  const bool success = OAuthRequestSigner::SignAuthHeader(
      request_base_url, parameters, OAuthRequestSigner::HMAC_SHA1_SIGNATURE,
      http_method == HTTP_METHOD_GET ? OAuthRequestSigner::GET_METHOD
                                     : OAuthRequestSigner::POST_METHOD,
      client_key_, client_secret_, auth_data_.token, auth_data_.token_secret,
      &result);
  DCHECK(success);

  // strlen("OAuth ") == 6
  result.insert(6, "realm=\"" + realm + "\", ");
  return result;
}

void AccountServiceImpl::OnAuthDataFetched(
    const AccountAuthData& auth_data,
    opera_sync::OperaAuthProblem problem) {
  DCHECK(CalledOnValidThread());
  DCHECK(!auth_data.token.empty());
  DCHECK(!auth_data.token_secret.empty());
  auth_data_ = auth_data;
  VLOG(1) << "Authorization data obtained: " << auth_data.token;

  FOR_EACH_OBSERVER(AccountObserver, observers_, OnLoggedIn(this, problem));
}

void AccountServiceImpl::OnAuthDataFetchFailed(
    account_util::AuthDataUpdaterError error_code,
    account_util::AuthOperaComError auth_code,
    const std::string& message,
    opera_sync::OperaAuthProblem problem) {
  DCHECK(CalledOnValidThread());
  DCHECK_NE(error_code, account_util::ADUE_NO_ERROR);
  // XOR
  DCHECK((error_code == account_util::ADUE_AUTH_ERROR) !=
         (auth_code == account_util::AOCE_NO_ERROR));

  auth_data_ = AccountAuthData();
  FOR_EACH_OBSERVER(
      AccountObserver, observers_,
      OnLoginError(this, error_code, auth_code, message, problem));
}

bool AccountServiceImpl::HasAuthToken() const {
  DCHECK(CalledOnValidThread());
  return !auth_data_.token.empty() && !auth_data_.token_secret.empty();
}

base::Time AccountServiceImpl::next_token_request_time() const {
  DCHECK(opera::SyncConfig::ShouldUseAuthTokenRecovery());
  return next_token_request_time_;
}

bool AccountServiceImpl::NotifyIfLoggedIn(
    opera_sync::OperaAuthProblem problem) {
  DCHECK(CalledOnValidThread());
  if (!HasAuthToken())
    return false;

  OnAuthDataFetched(auth_data_, problem);
  return true;
}

bool AccountServiceImpl::ShouldDelayTokenRequest() const {
  DCHECK(opera::SyncConfig::ShouldUseAuthTokenRecovery());
  if (DeltaTillNextTokenRequestAllowed() == base::TimeDelta())
    return false;
  return true;
}

base::TimeDelta AccountServiceImpl::DeltaTillNextTokenRequestAllowed() const {
  DCHECK(opera::SyncConfig::ShouldUseAuthTokenRecovery());
  base::TimeDelta delta_from_last_request =
      base::Time::Now() - last_token_request_time_;

  if (delta_from_last_request >= retry_timeout_)
    return base::TimeDelta();

  return retry_timeout_ - delta_from_last_request;
}

void AccountServiceImpl::SetRetryTimeout(const base::TimeDelta& delta) {
  retry_timeout_ = delta;
}

void AccountServiceImpl::DoRequestAuthData(
    opera_sync::OperaAuthProblem problem) {
  if (opera::SyncConfig::ShouldUseAuthTokenRecovery()) {
    if (problem.source() != opera_sync::OperaAuthProblem::SOURCE_LOGIN)
      last_token_request_time_ = base::Time::Now();

    next_token_request_time_ = base::Time();
  }

  delegate_->RequestAuthData(
      problem, base::Bind(&AccountService::OnAuthDataFetched, AsWeakPtr()),
      base::Bind(&AccountService::OnAuthDataFetchFailed, AsWeakPtr()));
}

}  // namespace opera
