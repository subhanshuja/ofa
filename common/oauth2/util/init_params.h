// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef COMMON_OAUTH2_UTIL_INIT_PARAMS_H_
#define COMMON_OAUTH2_UTIL_INIT_PARAMS_H_

#include <memory>
#include <string>

#include "base/time/tick_clock.h"
#include "components/prefs/pref_service.h"

#include "common/oauth2/device_name/device_name_service.h"
#include "common/oauth2/diagnostics/diagnostic_service.h"
#include "common/oauth2/migration/oauth1_migrator.h"
#include "common/oauth2/network/network_request_manager.h"
#include "common/oauth2/session/persistent_session.h"
#include "common/oauth2/token_cache/token_cache.h"

namespace opera {
namespace oauth2 {

class InitParams {
 public:
  InitParams();
  ~InitParams();

  bool IsValid() const;

  base::TickClock* backoff_clock;
  std::string client_id;
  DeviceNameService* device_name_service;
  DiagnosticService* diagnostic_service;
  std::unique_ptr<OAuth1Migrator> oauth1_migrator;
  std::unique_ptr<NetworkRequestManager> oauth2_network_request_manager;
  TokenCache* oauth2_token_cache;
  std::unique_ptr<PersistentSession> oauth2_session;
  PrefService* pref_service;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner;
};

}  // namespace oauth2
}  // namespace opera
#endif  // COMMON_OAUTH2_UTIL_INIT_PARAMS_H_
