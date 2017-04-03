// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_SYNC_SYNC_TYPES_H_
#define COMMON_SYNC_SYNC_TYPES_H_

#include <memory>

#include "base/callback.h"
#include "base/time/time.h"

#include "common/sync/sync_account.h"
#include "common/sync/sync_login_data_store.h"

class Profile;

namespace opera {

namespace oauth2 {
class AuthService;
class DiagnosticService;
}

enum SyncLoginUIParams {
  /**
   * Show an option to suppress automatic log-in screen in the future.
   */
  SYNC_LOGIN_UI_WITH_SUPPRESS_OPTION,

  /**
   * Don't show any option to suppress automatic log-in screen in the future.
   */
  SYNC_LOGIN_UI_WITHOUT_SUPPRESS_OPTION,
};

typedef base::Callback<void(SyncLoginUIParams)> ShowSyncLoginPromptCallback;


enum SyncConnectionStatus {
  SYNC_CONNECTION_LOST,
  SYNC_CONNECTION_RESTORED,
  SYNC_CONNECTION_SERVER_ERROR,
};

typedef base::Callback<void(int status,
                            Profile* profile,
                            const ShowSyncLoginPromptCallback& prompt_callback)>
    ShowSyncConnectionStatusCallback;

typedef base::Callback<std::unique_ptr<TimeSkewResolver>()> TimeSkewResolverFactory;

struct OperaSyncInitParams {
  OperaSyncInitParams();
  ~OperaSyncInitParams();
  OperaSyncInitParams(OperaSyncInitParams&& other);  // NOLINT

  /**
   * Whether it's allowed to create ProfileSyncService.
   */
  bool sync_enabled;
  /**
   * Should show the log-in UI.
   */
  ShowSyncLoginPromptCallback show_login_prompt;
  /**
   * Should display information about the connection status.
   */
  ShowSyncConnectionStatusCallback show_connection_status;
  std::unique_ptr<SyncLoginDataStore> login_data_store;
  SyncAccount::AuthDataUpdaterFactory auth_data_updater_factory;
  TimeSkewResolverFactory time_skew_resolver_factory;
  base::TimeDelta network_error_show_status_delay;
  oauth2::AuthService* auth_service;
  oauth2::DiagnosticService* diagnostic_service;

 private:
  DISALLOW_COPY_AND_ASSIGN(OperaSyncInitParams);
};

}  // namespace opera

#endif  // COMMON_SYNC_SYNC_TYPES_H_
