// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/permissions/permission_callback.h"

#include "chill/browser/permissions/permission_status.h"

namespace opera {

void PermissionCallback::Run(const std::vector<PermissionStatus>& status) {
  std::vector<blink::mojom::PermissionStatus> result(status.size());
  for (size_t i = 0; i < status.size(); i++) {
    result[i] = Convert(status[i]);
  }
  callback_.Run(result);
}

}  // namespace opera
