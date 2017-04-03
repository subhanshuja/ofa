// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/content/shell/browser/shell_permission_manager.cc
// @synchronized 59c61dd678d451d5754318e2060a3f4c91d69d1f

#include "chill/browser/permissions/opera_permission_manager.h"

#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

#include "chill/app/native_interface.h"
#include "chill/browser/chromium_tab_delegate.h"
#include "chill/browser/permissions/permission_bridge.h"
#include "chill/browser/permissions/permission_callback.h"
#include "chill/browser/permissions/permission_status.h"

namespace {

void CallbackAdapter(
    const base::Callback<void(blink::mojom::PermissionStatus)> callback,
    const std::vector<blink::mojom::PermissionStatus>& result) {
  callback.Run(result[0]);
}

}

namespace opera {

OperaPermissionManager::OperaPermissionManager(bool private_mode) {
  bridge_ = NativeInterface::Get()->GetPermissionBridge(private_mode);
}

int OperaPermissionManager::RequestPermission(
    content::PermissionType permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    const base::Callback<void(blink::mojom::PermissionStatus)>& callback) {
  std::vector<content::PermissionType> permissions(1, permission);
  return RequestPermissions(permissions, render_frame_host, requesting_origin,
      user_gesture, base::Bind(&CallbackAdapter, callback));
}

int OperaPermissionManager::RequestPermissions(
    const std::vector<content::PermissionType>& permission,
    content::RenderFrameHost* render_frame_host,
    const GURL& requesting_origin,
    bool user_gesture,
    const base::Callback<void(const std::vector<blink::mojom::PermissionStatus>&)>&
        callback) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  return RequestPermissions(permission, web_contents, requesting_origin,
      callback);
}

int OperaPermissionManager::RequestPermissions(
    const std::vector<content::PermissionType>& permission,
    content::WebContents* web_contents,
    const GURL& requesting_origin,
    const base::Callback<void(const std::vector<blink::mojom::PermissionStatus>&)>&
        callback) {
  ChromiumTabDelegate* delegate =
      ChromiumTabDelegate::FromWebContents(web_contents);

  DCHECK(delegate);
  if (!delegate) {
    std::vector<blink::mojom::PermissionStatus> deny(
        permission.size(), blink::mojom::PermissionStatus::DENIED);
    callback.Run(deny);
    return kNoPendingOperation;
  }

  DCHECK(web_contents);
  GURL embedding_origin = web_contents->GetLastCommittedURL().GetOrigin();

  return bridge_->RequestPermissions(
      delegate->GetChromiumTab(),
      permission,
      requesting_origin.GetOrigin().spec(),
      embedding_origin.spec(),
      new PermissionCallback(callback));
}

void OperaPermissionManager::CancelPermissionRequest(int request_id) {
  bridge_->CancelPermissionRequest(request_id);
}

void OperaPermissionManager::ResetPermission(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
  bridge_->ResetPermission(permission,
      requesting_origin.GetOrigin().spec());
}

blink::mojom::PermissionStatus OperaPermissionManager::GetPermissionStatus(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
  return Convert(bridge_->GetPermissionStatus(permission,
      requesting_origin.GetOrigin().spec()));
}

void OperaPermissionManager::RegisterPermissionUsage(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
}

int OperaPermissionManager::SubscribePermissionStatusChange(
    content::PermissionType permission,
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    const base::Callback<void(blink::mojom::PermissionStatus)>& callback) {
  return bridge_->SubscribePermissionStatusChange(permission,
      requesting_origin.GetOrigin().spec(),
      new PermissionCallback(base::Bind(&CallbackAdapter, callback)));
}

void OperaPermissionManager::UnsubscribePermissionStatusChange(
    int subscription_id) {
  bridge_->UnsubscribePermissionStatusChange(subscription_id);
}

}  // namespace opera
