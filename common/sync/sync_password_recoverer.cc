// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sync_password_recoverer.h"

#include <vector>

#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "components/autofill/core/common/password_form.h"

#include "common/account/account_observer.h"
#include "common/account/account_service.h"
#include "common/account/time_skew_resolver.h"
#include "common/sync/sync_auth_data_updater.h"
#include "common/sync/sync_config.h"
#include "common/sync/sync_login_data.h"
#include "common/sync/sync_login_data_store.h"

namespace {
const int kDefaultPasswordStoreTimeoutMs = 5000;

opera::AccountService* CreateAccountService(
    opera::AccountServiceDelegate* delegate) {
  DCHECK(!opera::SyncConfig::UsesOAuth2());
  return opera::AccountService::Create(opera::SyncConfig::ClientKey(),
                                       opera::SyncConfig::ClientSecret(),
                                       delegate);
}

void StopAndSuppress() {}
}  // namespace

namespace opera {
SyncPasswordRecoverer::SyncPasswordRecoverer(Profile* profile)
    : state_(IDLE),
      result_(SPRR_UNKNOWN),
      profile_(profile),
      password_store_timeout_ms_(kDefaultPasswordStoreTimeoutMs) {
  DCHECK(!SyncConfig::UsesOAuth2());
  DCHECK(profile);
}

SyncPasswordRecoverer::~SyncPasswordRecoverer() {
  DCHECK(!SyncConfig::UsesOAuth2());
}

bool SyncPasswordRecoverer::TryRecover(
    const std::string& sync_username,
    RecoveryFinishedCallback finished_callback) {
  DCHECK(!SyncConfig::UsesOAuth2());
  VLOG(1) << "Start recovery.";

  if (sync_username.empty()) {
    VLOG(1) << "Empty username!";
    return false;
  }

  if (state_ != IDLE || result_ != SPRR_UNKNOWN) {
    VLOG(1) << "Ignoring (" << state_ << ") (" << result_ << ") '"
            << sync_username_ << "'";
    return false;
  }

  DCHECK(finished_callback_.is_null());
  finished_callback_ = finished_callback;
  result_ = SPRR_UNKNOWN;
  state_ = WAIT_FOR_PASSWORD_MANAGER;
  password_.clear();
  token_.clear();
  token_secret_.clear();
  sync_username_ = base::UTF8ToUTF16(sync_username);
  password_store_ = PasswordStoreFactory::GetForProfile(
      profile_, ServiceAccessType::EXPLICIT_ACCESS);
  this->AddRef();
  if (!password_store_.get()) {
    result_ = SPRR_INTERNAL_ERROR;
    state_ = IDLE;
    Notify();
    return false;
  }
  VLOG(1) << "Fetch data.";
  timer_.Start(FROM_HERE,
               base::TimeDelta::FromMilliseconds(password_store_timeout_ms_),
               this, &SyncPasswordRecoverer::OnPasswordStoreRequestTimeout);
  password_store_->GetAutofillableLogins(this);
  return true;
}

std::string SyncPasswordRecoverer::user_name() const {
  DCHECK(!SyncConfig::UsesOAuth2());
  return base::UTF16ToUTF8(sync_username_);
}

void SyncPasswordRecoverer::OnPasswordStoreRequestTimeout() {
  DCHECK(!SyncConfig::UsesOAuth2());
  DCHECK_EQ(state_, WAIT_FOR_PASSWORD_MANAGER);
  result_ = SPRR_PASSWORD_MANAGER_TIMEOUT;
  state_ = IDLE;
  Notify();
}

void SyncPasswordRecoverer::OnGetPasswordStoreResults(
    std::vector<std::unique_ptr<autofill::PasswordForm>> results) {
  DCHECK(!SyncConfig::UsesOAuth2());
  timer_.Stop();
  if (result_ == SPRR_PASSWORD_MANAGER_TIMEOUT) {
    VLOG(1) << "Ignoring.";
    // This request did timeout in the meantime, all we can do is decrease
    // the refcount above.
    DCHECK_EQ(state_, IDLE);
    return;
  }

  DCHECK_EQ(state_, WAIT_FOR_PASSWORD_MANAGER);
  DCHECK(!sync_username_.empty());
  DCHECK(password_.empty());
  VLOG(1) << "Got data.";

  std::vector<std::string> login_paths;
  login_paths.push_back(PathToAuthURL("/account/login"));
  login_paths.push_back(PathToAuthURL("/account/signup"));

  for (auto i = results.begin(); i != results.end(); i++) {
    const std::string& origin_url = (*i)->origin.spec();
    const base::string16& username = (*i)->username_value;

    bool path_valid = (std::find(login_paths.begin(), login_paths.end(),
                                 origin_url) != login_paths.end());
    if (path_valid && username == sync_username_) {
      password_ = base::UTF16ToUTF8((*i)->password_value);
      break;
    }
  }

  if (password_.empty()) {
    result_ = SPRR_CREDENTIALS_NOT_FOUND;
    state_ = IDLE;
    Notify();
    return;
  }
  state_ = WAIT_FOR_ACCOUNT;
  TryLogin();
}

void SyncPasswordRecoverer::TryLogin() {
  DCHECK(!SyncConfig::UsesOAuth2());
  DCHECK_EQ(state_, WAIT_FOR_ACCOUNT);
  DCHECK(!sync_username_.empty());
  DCHECK(!password_.empty());

  std::unique_ptr<opera::TimeSkewResolver> null_time_skew_resolver;
  std::unique_ptr<opera::SyncLoginDataStore> null_sync_login_data_store;
  opera::SyncAccount::AuthDataUpdaterFactory factory =
      base::Bind(&opera::CreateAuthDataUpdater,
                 base::Unretained(profile_->GetRequestContext()));

  opera::ExternalCredentials credentials(base::UTF16ToUTF8(sync_username_),
                                         password_);
  recoverer_account_ =
      opera::SyncAccount::Create(
          profile_->GetRequestContext(), base::Bind(&CreateAccountService),
          std::move(null_time_skew_resolver),
          std::move(null_sync_login_data_store),
          factory, base::Bind(&StopAndSuppress), credentials);
  DCHECK(recoverer_account_.get());
  recoverer_account_->service()->AddObserver(this);
  VLOG(1) << "Start check.";
  recoverer_account_->LogIn();
}

std::string SyncPasswordRecoverer::PathToAuthURL(const std::string& path) {
  DCHECK(!SyncConfig::UsesOAuth2());
  return opera::SyncConfig::AuthServerURL().GetOrigin().Resolve(path).spec();
}

void SyncPasswordRecoverer::OnLoggedIn(opera::AccountService* service,
                                       opera_sync::OperaAuthProblem problem) {
  DCHECK(!SyncConfig::UsesOAuth2());
  DCHECK_EQ(state_, WAIT_FOR_ACCOUNT);
  DCHECK_EQ(service, recoverer_account_->service().get());
  DCHECK(token_.empty());
  DCHECK(token_secret_.empty());
  VLOG(1) << "OK";

  result_ = SPRR_RECOVERED;
  state_ = IDLE;
  token_ = recoverer_account_->login_data().auth_data.token;
  token_secret_ = recoverer_account_->login_data().auth_data.token_secret;
  DCHECK(!token_.empty());
  DCHECK(!token_secret_.empty());
  Notify();
}

void SyncPasswordRecoverer::OnLoginError(
    opera::AccountService* service,
    opera::account_util::AuthDataUpdaterError error_code,
    opera::account_util::AuthOperaComError auth_code,
    const std::string& message,
    opera_sync::OperaAuthProblem problem) {
  DCHECK(!SyncConfig::UsesOAuth2());
  DCHECK_EQ(state_, WAIT_FOR_ACCOUNT);
  DCHECK_EQ(service, recoverer_account_->service().get());
  VLOG(1) << "Failed: "
          << opera::account_util::AuthDataUpdaterErrorToString(error_code)
          << " '" << message << "' " << problem.ToString();
  password_.clear();

  switch (error_code) {
    case account_util::ADUE_AUTH_ERROR:
      DCHECK_EQ(auth_code, account_util::AOCE_AUTH_ERROR_SIMPLE_REQUEST);
      result_ = SPRR_CREDENTIALS_INCORRECT;
      break;
    case account_util::ADUE_PARSE_ERROR:
      DCHECK_EQ(auth_code, account_util::AOCE_NO_ERROR);
      result_ = SPRR_PARSE_ERROR;
      break;
    case account_util::ADUE_HTTP_ERROR:
      DCHECK_EQ(auth_code, account_util::AOCE_NO_ERROR);
      result_ = SPRR_HTTP_ERROR;
      break;
    case account_util::ADUE_NETWORK_ERROR:
      DCHECK_EQ(auth_code, account_util::AOCE_NO_ERROR);
      result_ = SPRR_NETWORK_ERROR;
      break;
    case account_util::ADUE_INTERNAL_ERROR:
    // We should not ever get this from the simple token fetcher.
    // Fall through.
    default:
      NOTREACHED();
      result_ = SPRR_INTERNAL_ERROR;
      break;
  }
  state_ = IDLE;
  Notify();
}

void SyncPasswordRecoverer::OverridePasswordStoreTimeoutForTest(
    int new_timeout_ms) {
  DCHECK(!SyncConfig::UsesOAuth2());
  DCHECK_EQ(state_, IDLE);
  DCHECK_EQ(result_, SPRR_UNKNOWN);
  password_store_timeout_ms_ = new_timeout_ms;
}

void SyncPasswordRecoverer::Notify() {
  DCHECK(!SyncConfig::UsesOAuth2());
  DCHECK(!finished_callback_.is_null());
  DCHECK_EQ(state_, IDLE);
  VLOG(1) << "Notify " << result_ << " has password = " << (!password_.empty())
          << " has token = " << (!token_.empty())
          << " has secret = " << (!token_secret_.empty());
  if (result_ == SPRR_RECOVERED) {
    DCHECK(!password_.empty());
    DCHECK(!token_.empty());
    DCHECK(!token_secret_.empty());
  } else {
    DCHECK(password_.empty());
    DCHECK(token_.empty());
    DCHECK(token_secret_.empty());
  }
  finished_callback_.Run(this, result_, user_name(), password_, token_,
                         token_secret_);
  finished_callback_.Reset();
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(&SyncPasswordRecoverer::OnPostNotify, this));
}

void SyncPasswordRecoverer::OnPostNotify() {
  DCHECK(!SyncConfig::UsesOAuth2());
  this->Release();
}

const std::string SyncPasswordRecovererResultToString(
    SyncPasswordRecoverer::Result result) {
  DCHECK(!SyncConfig::UsesOAuth2());
  switch (result) {
    case SyncPasswordRecoverer::SPRR_UNKNOWN:
      return "UNKNOWN";
    case SyncPasswordRecoverer::SPRR_RECOVERED:
      return "RECOVERED";
    case SyncPasswordRecoverer::SPRR_INTERNAL_ERROR:
      return "INTERNAL_ERROR";
    case SyncPasswordRecoverer::SPRR_CREDENTIALS_NOT_FOUND:
      return "CREDENTIALS_NOT_FOUND";
    case SyncPasswordRecoverer::SPRR_CREDENTIALS_INCORRECT:
      return "CREDENTIALS_INCORRECT";
    case SyncPasswordRecoverer::SPRR_HTTP_ERROR:
      return "HTTP_ERROR";
    case SyncPasswordRecoverer::SPRR_NETWORK_ERROR:
      return "NETWORK_ERROR";
    case SyncPasswordRecoverer::SPRR_PARSE_ERROR:
      return "PARSE_ERROR";
    case SyncPasswordRecoverer::SPRR_PASSWORD_MANAGER_TIMEOUT:
      return "PASSWORD_MANAGER_TIMEOUT";
    default:
      NOTREACHED();
      return "<UNKNOWN>";
  }
}

}  // namespace opera
