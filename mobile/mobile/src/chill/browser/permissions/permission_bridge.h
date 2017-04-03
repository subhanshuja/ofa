// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2014 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_PERMISSIONS_PERMISSION_BRIDGE_H_
#define CHILL_BROWSER_PERMISSIONS_PERMISSION_BRIDGE_H_

namespace content {
enum class PermissionType;
}

namespace opera {

class PermissionCallback;
enum class PermissionStatus;

class PermissionBridge {
 public:
  virtual ~PermissionBridge() {}

  // Takes ownership of callback.
  virtual int RequestPermissions(
      jobject chromium_tab,
      const std::vector<content::PermissionType>& permissions,
      std::string origin,
      std::string embedding_origin,
      PermissionCallback* callback) = 0;

  virtual void CancelPermissionRequest(
      int request_id) = 0;

  virtual PermissionStatus GetPermissionStatus(
      content::PermissionType permission,
      std::string origin) = 0;

  virtual void ResetPermission(
      content::PermissionType permission,
      std::string origin) = 0;

  virtual int SubscribePermissionStatusChange(
      content::PermissionType permission,
      std::string origin,
      PermissionCallback* callback) = 0;

  virtual void UnsubscribePermissionStatusChange(int subscription_id) = 0;
};

}  // namespace opera

#endif  // CHILL_BROWSER_PERMISSIONS_PERMISSION_BRIDGE_H_
