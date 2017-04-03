// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/browser/shell_permission_manager.h
// @synchronized 59c61dd678d451d5754318e2060a3f4c91d69d1f

#ifndef CHILL_BROWSER_PERMISSIONS_OPERA_PERMISSION_MANAGER_H_
#define CHILL_BROWSER_PERMISSIONS_OPERA_PERMISSION_MANAGER_H_

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/public/browser/permission_manager.h"

namespace content {
class WebContents;
}

namespace opera {

class PermissionBridge;

class OperaPermissionManager : public content::PermissionManager {
 public:
  explicit OperaPermissionManager(bool private_mode);
  ~OperaPermissionManager() override {}

  // PermissionManager implementation.
  int RequestPermission(
      content::PermissionType permission,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      bool user_gesture,
      const base::Callback<void(blink::mojom::PermissionStatus)>& callback) override;
  int RequestPermissions(
      const std::vector<content::PermissionType>& permission,
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      bool user_gesture,
      const base::Callback<void(const std::vector<blink::mojom::PermissionStatus>&)>&
          callback) override;
  void CancelPermissionRequest(int request_id) override;
  void ResetPermission(content::PermissionType permission,
                       const GURL& requesting_origin,
                       const GURL& embedding_origin) override;
  blink::mojom::PermissionStatus GetPermissionStatus(
      content::PermissionType permission,
      const GURL& requesting_origin,
      const GURL& embedding_origin) override;
  void RegisterPermissionUsage(content::PermissionType permission,
                               const GURL& requesting_origin,
                               const GURL& embedding_origin) override;
  virtual int SubscribePermissionStatusChange(
      content::PermissionType permission,
      const GURL& requesting_origin,
      const GURL& embedding_origin,
      const base::Callback<void(blink::mojom::PermissionStatus)>& callback);
  virtual void UnsubscribePermissionStatusChange(int subscription_id);

  // Opera extension.
  int RequestPermissions(
      const std::vector<content::PermissionType>& permission,
      content::WebContents* web_contents,
      const GURL& requesting_origin,
      const base::Callback<void(const std::vector<blink::mojom::PermissionStatus>&)>&
          callback);

 private:
  PermissionBridge* bridge_;

  DISALLOW_COPY_AND_ASSIGN(OperaPermissionManager);
};

}  // namespace opera

#endif // CHILL_BROWSER_PERMISSIONS_OPERA_PERMISSION_MANAGER_H_
