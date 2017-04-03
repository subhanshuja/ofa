// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_PERMISSIONS_PERMISSION_CALLBACK_H_
#define CHILL_BROWSER_PERMISSIONS_PERMISSION_CALLBACK_H_

#include <vector>

#include "base/callback.h"
#include "third_party/WebKit/public/platform/modules/permissions/permission_status.mojom.h"

namespace opera {

enum class PermissionStatus;

class PermissionCallback {
 public:
  explicit PermissionCallback(
      const base::Callback<void(const std::vector<blink::mojom::PermissionStatus>&)>&
          callback)
      : callback_(callback) {}

  void Run(const std::vector<PermissionStatus>& status);

 private:
  const base::Callback<void(const std::vector<blink::mojom::PermissionStatus>&)>
      callback_;
};

}  // namespace opera

#endif  // CHILL_BROWSER_PERMISSIONS_PERMISSION_CALLBACK_H_
