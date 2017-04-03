// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/chrome_quota_permission_context.h

#ifndef CHILL_BROWSER_OPERA_QUOTA_PERMISSION_CONTEXT_H_
#define CHILL_BROWSER_OPERA_QUOTA_PERMISSION_CONTEXT_H_

#include "base/compiler_specific.h"
#include "content/public/browser/quota_permission_context.h"
#include "content/public/common/storage_quota_params.h"

namespace opera {

class OperaQuotaPermissionContext : public content::QuotaPermissionContext {
 public:
  OperaQuotaPermissionContext();

  // The callback will be dispatched on the IO thread.
  virtual void RequestQuotaPermission(
      const content::StorageQuotaParams& params,
      int render_process_id,
      const PermissionCallback& callback) override;

  static void DispatchCallbackOnIOThread(
      const PermissionCallback& callback,
      QuotaPermissionResponse response);

  static void RequestDialogOnUIThread(
      int render_process_id,
      int render_view_id,
      const PermissionCallback& callback);

 private:
  virtual ~OperaQuotaPermissionContext();
};

}  // namespace opera

#endif  // CHILL_BROWSER_OPERA_QUOTA_PERMISSION_CONTEXT_H_
