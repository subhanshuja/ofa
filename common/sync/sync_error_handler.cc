// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sync_error_handler.h"

#include "base/bind.h"
#include "components/sync/driver/sync_service.h"

#include "common/sync/sync_delay_provider.h"
#include "common/sync/sync_status.h"

namespace opera {

SyncErrorHandler::SyncErrorHandler(
    Profile* profile,
    base::TimeDelta retry_delay,
    std::unique_ptr<SyncDelayProvider> delay_provider,
    const ShowSyncConnectionStatusCallback& show_status_callback,
    const ShowSyncLoginPromptCallback& show_login_prompt_callback)
    : profile_(profile),
      retry_delay_(retry_delay),
      delay_provider_(std::move(delay_provider)),
      show_connection_status_callback_(show_status_callback),
      show_login_prompt_callback_(show_login_prompt_callback),
      should_show_connection_restored_(false),
      was_enabled_(false) {
}

SyncErrorHandler::~SyncErrorHandler() {
}

void SyncErrorHandler::OnSyncStatusChanged(
    syncer::SyncService* sync_service) {
  const SyncStatus status = sync_service->opera_sync_status();
  if (status.network_error()) {
    if (!should_show_connection_restored_ &&
        !delay_provider_->IsScheduled()) {
      delay_provider_->InvokeAfter(
          retry_delay_,
          base::Bind(&SyncErrorHandler::ShowConnectionLost,
                     base::Unretained(this)));
    }
  } else {
    delay_provider_->Cancel();

    if (status.enabled() && should_show_connection_restored_)
      show_connection_status_callback_.Run(
          SYNC_CONNECTION_RESTORED, profile_, show_login_prompt_callback_);

    should_show_connection_restored_ = false;

    if (was_enabled_ && status.server_error())
      show_connection_status_callback_.Run(
          SYNC_CONNECTION_SERVER_ERROR, profile_, show_login_prompt_callback_);
  }

  was_enabled_ = status.enabled();
}

void SyncErrorHandler::ShowConnectionLost() {
  show_connection_status_callback_.Run(
      SYNC_CONNECTION_LOST, profile_, show_login_prompt_callback_);
  should_show_connection_restored_ = true;
}

}  // namespace opera
