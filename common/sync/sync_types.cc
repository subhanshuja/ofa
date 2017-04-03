// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/sync_types.h"

namespace opera {

OperaSyncInitParams::OperaSyncInitParams()
    : sync_enabled(true), auth_service(nullptr), diagnostic_service(nullptr) {}

OperaSyncInitParams::~OperaSyncInitParams() {}

OperaSyncInitParams::OperaSyncInitParams(OperaSyncInitParams&& other)  // NOLINT
    : sync_enabled(other.sync_enabled),
      show_login_prompt(std::move(other.show_login_prompt)),
      show_connection_status(std::move(other.show_connection_status)),
      login_data_store(std::move(other.login_data_store)),
      auth_data_updater_factory(std::move(other.auth_data_updater_factory)),
      time_skew_resolver_factory(std::move(other.time_skew_resolver_factory)),
      network_error_show_status_delay(other.network_error_show_status_delay),
      auth_service(other.auth_service),
      diagnostic_service(other.diagnostic_service) {}

}  // namespace opera
