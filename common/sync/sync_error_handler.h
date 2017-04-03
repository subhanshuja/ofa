// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef DESKTOP_COMMON_SYNC_SYNC_ERROR_HANDLER_H_
#define DESKTOP_COMMON_SYNC_SYNC_ERROR_HANDLER_H_

#include "base/compiler_specific.h"
#include "base/time/time.h"

#include "common/sync/sync_delay_provider.h"
#include "common/sync/sync_observer.h"
#include "common/sync/sync_types.h"

class Profile;

namespace opera {

class SyncDelayProvider;

class SyncErrorHandler : public SyncObserver {
 public:
  SyncErrorHandler(
      Profile* profile,
      base::TimeDelta show_status_delay,
      std::unique_ptr<SyncDelayProvider> delay_provider,
      const ShowSyncConnectionStatusCallback& show_status_callback,
      const ShowSyncLoginPromptCallback& show_login_prompt_callback);
  ~SyncErrorHandler() override;

  /**
   * @name SyncObserver implementation
   * @{
   */
  void OnSyncStatusChanged(syncer::SyncService* sync_service) override;
  /** @} */

 private:
  void ShowConnectionLost();

  Profile* profile_;
  const base::TimeDelta retry_delay_;
  std::unique_ptr<SyncDelayProvider> delay_provider_;
  const ShowSyncConnectionStatusCallback show_connection_status_callback_;
  const ShowSyncLoginPromptCallback show_login_prompt_callback_;
  bool should_show_connection_restored_;
  bool was_enabled_;
};

}  // namespace opera

#endif  // DESKTOP_COMMON_SYNC_SYNC_ERROR_HANDLER_H_
