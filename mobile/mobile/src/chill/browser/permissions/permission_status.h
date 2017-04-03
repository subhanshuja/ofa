// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_PERMISSIONS_PERMISSION_STATUS_H_
#define CHILL_BROWSER_PERMISSIONS_PERMISSION_STATUS_H_

#include "third_party/WebKit/public/platform/modules/permissions/permission_status.mojom.h"

namespace opera {

// The content::PermissionStatus enum is generated when compiling Chromium,
// using it in the PermissionBridge as-is would cause a circular dependency
// with SWIG. Thus we have to create our own "fixed" PermissionStatus enum
// and translate between them.

enum class PermissionStatus {
  GRANTED,
  DENIED,
  ASK,
};

inline blink::mojom::PermissionStatus Convert(PermissionStatus status) {
  switch (status) {
    case PermissionStatus::GRANTED:
      return blink::mojom::PermissionStatus::GRANTED;
    case PermissionStatus::DENIED:
      return blink::mojom::PermissionStatus::DENIED;
    case PermissionStatus::ASK:
      return blink::mojom::PermissionStatus::ASK;
  }
  return blink::mojom::PermissionStatus::DENIED;
}

inline PermissionStatus Convert(blink::mojom::PermissionStatus status) {
  switch (status) {
    case blink::mojom::PermissionStatus::GRANTED:
      return PermissionStatus::GRANTED;
    case blink::mojom::PermissionStatus::DENIED:
      return PermissionStatus::DENIED;
    case blink::mojom::PermissionStatus::ASK:
      return PermissionStatus::ASK;
  }
  return PermissionStatus::DENIED;
}

}  // namespace opera

#endif  // CHILL_BROWSER_PERMISSIONS_PERMISSION_STATUS_H_
