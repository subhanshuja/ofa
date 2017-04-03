// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "common/sync/invalidation.h"

namespace mobile {

Invalidation::Invalidation()
    : InvalidationData(SyncManager::SERVICE_GCM,
#if PUBLIC_BUILD
# if BETA_BUILD
                       SyncManager::BUILD_DEVELOPER
# else
                       SyncManager::BUILD_PUBLIC
# endif
#else
                       SyncManager::BUILD_INTERNAL
#endif
                       ), first_(true) {
}

Invalidation::~Invalidation() {
}

void Invalidation::SetDeviceId(const std::string& device_id) {
  if (device_id == GetDeviceId())
    return;
  if (first_) {
    first_ = false;
    Loaded(device_id);
  } else {
    Changed(device_id);
  }
}

}  // namespace mobile
