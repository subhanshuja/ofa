// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/authentication_dialog.h"

#include "base/android/jni_android.h"
#include "base/bind.h"
#include "base/logging.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/resource_dispatcher_host.h"
#include "content/public/browser/web_contents.h"
#include "net/base/auth.h"
#include "net/base/load_flags.h"
#include "net/url_request/url_request.h"

#include "chill/browser/chromium_tab.h"
#include "chill/browser/ui/login_interstitial_delegate.h"

using content::BrowserThread;

namespace opera {

AuthenticationDialogDelegate::AuthenticationDialogDelegate(
    net::URLRequest* request, jobject dialog) : request_(request) {
  // Keep a reference to the dialog (since there's no reference on the Java
  // side). This will be released as soon as the delegate is released from the
  // C++ side.
  j_dialog_.Reset(base::android::AttachCurrentThread(), dialog);
}

void AuthenticationDialogDelegate::OnRequestCancelled() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  request_ = NULL;
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&AuthenticationDialogDelegate::OnCancelled, this));
}

void AuthenticationDialogDelegate::OnShow() {
  // Implemented in Java
  NOTREACHED();
}

void AuthenticationDialogDelegate::OnCancelled() {
  // Implemented in Java
  NOTREACHED();
}

void AuthenticationDialogDelegate::SetOwner(ChromiumTab* tab) {
  // Implemented in Java
  NOTREACHED();
}

void AuthenticationDialogDelegate::ShowDialog(int render_process_id,
                                              int render_view_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  ChromiumTab* tab = ChromiumTab::FromID(render_process_id, render_view_id);

  content::RenderFrameHost* rfh =
      content::RenderFrameHost::FromID(render_process_id, render_view_id);
  // NOTE: web_contents may be null, e.g. when resuming a download from
  // the download manager.
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(rfh);

  const bool is_main_frame =
      (request_->load_flags() & net::LOAD_MAIN_FRAME_DEPRECATED) != 0;
  // NOTE: with instant back enabled all request are effectively
  // cross-origin. check is left here for the glorious day when we
  // remove instant back.
  const bool cross_origin = web_contents &&
                            web_contents->GetLastCommittedURL().GetOrigin() !=
                                request_->url().GetOrigin();

  if (is_main_frame && cross_origin) {
    // Show a blank interstitial for main-frame, cross origin requests
    // so that the correct URL is shown in the omnibox.
    base::Closure callback =
        base::Bind(&AuthenticationDialogDelegate::ShowDialogInternal,
                   this, tab);
    // This delegate creates an Interstitial that owns the delegate
    new LoginInterstitialDelegate(web_contents, request_->url(), callback);
  } else {
    ShowDialogInternal(tab);
  }
}

void AuthenticationDialogDelegate::ShowDialogInternal(ChromiumTab* tab) {
  // Notify the tab that http authentication has been requested. A NULL tab is
  // expected when resuming a SSL hosted download; for that case we immediately
  // show the authentication dialog.
  if (tab) {
    SetOwner(tab);
    tab->OnAuthenticationDialogRequest(this);
  } else {
    OnShow();
  }
}

void AuthenticationDialogDelegate::SendAuthToRequester(
    bool success,
    const base::string16& username,
    const base::string16& password) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (request_) {
    if (success) {
      request_->SetAuth(net::AuthCredentials(username, password));
    } else {
      request_->CancelAuth();
    }
    content::ResourceDispatcherHost::Get()->ClearLoginDelegateForRequest(
        request_);
  }
}

void AuthenticationDialog::SetDelegate(AuthenticationDialogDelegate* delegate) {
  delegate_ = delegate;
}

void AuthenticationDialog::Accept(const base::string16& username,
                                  const base::string16& password) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(delegate_);
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&AuthenticationDialogDelegate::SendAuthToRequester,
                 delegate_, true, username, password));
  // The delegate references the Java dialog object. Clear the reference to the
  // delegate held by the dialog so that the dialog will be destroyed.
  delegate_ = NULL;
}

void AuthenticationDialog::Cancel() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK(delegate_);
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&AuthenticationDialogDelegate::SendAuthToRequester,
                 delegate_, false, base::string16(), base::string16()));
  // The delegate references the Java dialog object. Clear the reference to the
  // delegate held by the dialog so that the dialog will be destroyed.
  delegate_ = NULL;
}

}  // namespace opera
