// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/permissions/permission_bridge.h"
#include "chill/browser/permissions/permission_callback.h"
#include "chill/browser/permissions/permission_status.h"
%}

// Generate enum for content::PermissionType
%include "content/public/browser/permission_type.h"

namespace opera {

JAVA_PROXY_TAKES_OWNERSHIP(PermissionCallback);

enum class PermissionStatus {
  GRANTED,
  DENIED,
  ASK,
};

class PermissionCallback {
 public:
  void Run(const std::vector<PermissionStatus>& status);

 private:
  PermissionCallback();
};

}

%template(PermissionStatusVector) std::vector<opera::PermissionStatus>;

%feature("director", assumeoverride=1) opera::PermissionBridge;
%rename ("NativePermissionBridge") opera::PermissionBridge;
VECTOR(PermissionTypeVector, content::PermissionType);

%include "browser/permissions/permission_bridge.h"
