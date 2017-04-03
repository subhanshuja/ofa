// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include <string>

#include "common/sync/sync_account.h"

#include "base/logging.h"

#include "common/account/account_service.h"
#include "common/sync/sync_config.h"
#include "common/sync/sync_login_data.h"

namespace opera {

void SyncAccount::LogIn() {
  service()->LogIn();
}

void SyncAccount::LogInWithAuthData(const SyncLoginData& login_data) {
  DCHECK(!login_data.auth_data.token.empty());
  DCHECK(!login_data.auth_data.token_secret.empty());

  SetLoginData(login_data);
  service()->LogIn();
  if (opera::SyncConfig::ShouldUseAuthTokenRecovery()) {
    SyncEnabled();
  }
}

void SyncAccount::LogOut(account_util::LogoutReason logout_reason) {
  // Will call ServiceDelegate::LoggedOut().
  service()->LogOut(logout_reason);
}

void SyncAccount::AuthDataExpired(opera_sync::OperaAuthProblem problem) {
  VLOG(1) << "Authorization expired (" << problem.ToString() << ")";
  // TODO(mzajaczkowski): Perhaps there is a better way to do this,
  // perhaps having an OnLoggedIn() callback on token refresh is not
  // the best idea
  if (!HasValidAuthData())  // Not being invalidated already?
    return;

  service()->AuthDataExpired(problem);
  service()->LogIn(problem);
}

void SyncAccount::RetryAuthDataRenewal(opera_sync::OperaAuthProblem problem) {
  VLOG(1) << "Authorization request retry";
  problem.set_source(opera_sync::OperaAuthProblem::SOURCE_RETRY);
  service()->AuthDataExpired(problem);
  service()->LogIn(problem);
}
}  // opera
