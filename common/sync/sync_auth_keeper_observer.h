// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_AUTH_KEEPER_OBSERVER_H_
#define COMMON_SYNC_SYNC_AUTH_KEEPER_OBSERVER_H_

#include <string>

#include "common/account/account_observer.h"
#include "common/sync/sync_auth_keeper.h"

namespace opera {
namespace sync {
class SyncAuthKeeper;

class SyncAuthKeeperObserver : public AccountObserver {
 public:
  virtual void OnSyncAuthKeeperTokenStateChanged(
      SyncAuthKeeper* keeper,
      SyncAuthKeeper::TokenState state) = 0;

  virtual void OnSyncAuthKeeperStatusChanged(SyncAuthKeeper* keeper) = 0;

  // SyncAuthKeeper will forward the AccountObserver events after updating
  // its internal state according to the events. Any AccountObserver that
  // wants to rely on auth keeper state needs to subscribe to auth keeper
  // events instead of observing the account directly. The reason for this
  // is that we can't know the order in which the observers are notified
  // otherwise.
  void OnLoggedIn(AccountService* service,
                  opera_sync::OperaAuthProblem problem) override {}

  void OnLoginError(AccountService* service,
                            account_util::AuthDataUpdaterError error_code,
                            account_util::AuthOperaComError auth_code,
                            const std::string& message,
                            opera_sync::OperaAuthProblem problem) override {}

  void OnLoggedOut(AccountService* service,
                           account_util::LogoutReason logout_reason) override {}

  void OnAuthDataExpired(AccountService* service,
        opera_sync::OperaAuthProblem problem) override {}

  void OnLoginRequested(AccountService* service,
                                opera_sync::OperaAuthProblem problem) override {
  }

  void OnLogoutRequested(
      AccountService* service,
      account_util::LogoutReason logout_reason) override {}
};
}  // namespace sync
}  // namespace opera
#endif  // COMMON_SYNC_SYNC_AUTH_KEEPER_OBSERVER_H_
