// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Modified by Opera Software ASA
// @copied-from chromium/src/chrome/chrome_quota_permission_context.cc

#include "chill/browser/opera_quota_permission_context.h"

#include "base/bind.h"
#include "base/callback.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_view_host.h"
#include "url/gurl.h"
#include "storage/common/quota/quota_types.h"

#include "chill/browser/chromium_tab_delegate.h"
#include "chill/browser/chromium_tab.h"

using content::BrowserThread;

namespace opera {

OperaQuotaPermissionContext::OperaQuotaPermissionContext() {
}

OperaQuotaPermissionContext::~OperaQuotaPermissionContext() {
}

void OperaQuotaPermissionContext::RequestQuotaPermission(
    const content::StorageQuotaParams& params,
    int render_process_id,
    const PermissionCallback& callback) {
  if (params.storage_type != storage::kStorageTypePersistent) {
    // For now we only support requesting quota with this interface
    // for Persistent storage type.
    DispatchCallbackOnIOThread(callback, QUOTA_PERMISSION_RESPONSE_DISALLOW);
    return;
  }

  RequestDialogOnUIThread(render_process_id, params.render_view_id, callback);
}

class QuotaPermissionDelegate : public PermissionDialogDelegate {
 public:
  explicit QuotaPermissionDelegate(
      const content::QuotaPermissionContext::PermissionCallback& callback)
      : callback_(callback) {}

  void Allow(ChromiumTab* tab) override {
    OperaQuotaPermissionContext::DispatchCallbackOnIOThread(
        callback_,
        content::QuotaPermissionContext::QUOTA_PERMISSION_RESPONSE_ALLOW);
  }
  void Cancel(ChromiumTab* tab) override {
    OperaQuotaPermissionContext::DispatchCallbackOnIOThread(
        callback_,
        content::QuotaPermissionContext::QUOTA_PERMISSION_RESPONSE_CANCELLED);
  }
  void Disallow(ChromiumTab* tab) override {
    OperaQuotaPermissionContext::DispatchCallbackOnIOThread(
        callback_,
        content::QuotaPermissionContext::QUOTA_PERMISSION_RESPONSE_DISALLOW);
  }

 private:
  content::QuotaPermissionContext::PermissionCallback callback_;
};

void OperaQuotaPermissionContext::RequestDialogOnUIThread(
      int render_process_id,
      int render_view_id,
      const content::QuotaPermissionContext::PermissionCallback& callback) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&OperaQuotaPermissionContext::RequestDialogOnUIThread,
                   render_process_id,
                   render_view_id, callback));
    return;
  }

  content::RenderViewHost* render_view_host =
      content::RenderViewHost::FromID(render_process_id, render_view_id);

  if (!render_view_host)
    return;

  content::WebContents* web_contents =
      content::WebContents::FromRenderViewHost(render_view_host);

  if (!web_contents)
    return;

  ChromiumTab* tab = ChromiumTab::FromWebContents(web_contents);

  tab->GetDelegate()->RequestPermissionDialog(
      ChromiumTabDelegate::QuotaPermission,
      tab->GetWebContents()->GetURL().host(),
      new QuotaPermissionDelegate(callback));
}

void OperaQuotaPermissionContext::DispatchCallbackOnIOThread(
    const content::QuotaPermissionContext::PermissionCallback& callback,
    content::QuotaPermissionContext::QuotaPermissionResponse response) {
  DCHECK(!callback.is_null());

  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&OperaQuotaPermissionContext::DispatchCallbackOnIOThread,
                   callback, response));
    return;
  }

  callback.Run(response);
}

}  // namespace opera
