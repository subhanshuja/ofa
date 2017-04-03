// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2016 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/oauth2/util/init_params.h"

namespace opera {
namespace oauth2 {

InitParams::InitParams()
    : backoff_clock(nullptr),
      device_name_service(nullptr),
      diagnostic_service(nullptr),
      oauth2_token_cache(nullptr),
      pref_service(nullptr) {}

InitParams::~InitParams() {}

bool InitParams::IsValid() const {
  if (client_id.empty())
    return false;
  if (!device_name_service)
    return false;
  if (!oauth1_migrator)
    return false;
  if (!oauth2_network_request_manager)
    return false;
  if (!oauth2_session)
    return false;
  if (!oauth2_token_cache)
    return false;
  if (!pref_service)
    return false;
  if (!task_runner)
    return false;

  return true;
};

}  // namespace oauth2
}  // namespace opera
