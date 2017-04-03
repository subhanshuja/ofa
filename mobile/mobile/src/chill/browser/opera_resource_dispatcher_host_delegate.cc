// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#include "chill/browser/opera_resource_dispatcher_host_delegate.h"

#include "base/bind.h"
#include "base/memory/scoped_vector.h"
#include "components/navigation_interception/intercept_navigation_delegate.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/resource_throttle.h"
#include "net/base/auth.h"
#include "url/gurl.h"

#include "chill/browser/authentication_dialog.h"

using navigation_interception::InterceptNavigationDelegate;

namespace content {
class URLRequest;
}  // namespace content

namespace opera {

void OperaResourceDispatcherHostDelegate::RequestBeginning(
    net::URLRequest* request,
    content::ResourceContext* resource_context,
    content::AppCacheService* appcache_service,
    content::ResourceType resource_type,
    ScopedVector<content::ResourceThrottle>* throttles) {
  if (resource_type != content::RESOURCE_TYPE_MAIN_FRAME) {
    InterceptNavigationDelegate::UpdateUserGestureCarryoverInfo(request);
  }
}

content::ResourceDispatcherHostLoginDelegate*
OperaResourceDispatcherHostDelegate::CreateLoginDelegate(
    net::AuthChallengeInfo* auth_info,
    net::URLRequest* request) {
  AuthenticationDialogDelegate* delegate = CreateAuthenticationDialog(
      auth_info->challenger.Serialize(), auth_info->realm, request);
  // We must have a reference to delegate here. Otherwise it will be destroyed
  // if the task posted below, which stores a reference to the delegate, is run
  // before this method returns. The caller will then never get a chance to
  // store its own reference. AuthenticationDialog::SetDelegate should have
  // been called at this point and AuthenticationDialog::delegate_ is a
  // scoped_refptr.
  DCHECK(delegate->HasOneRef());

  int render_process_id, render_frame_id;
  if (!content::ResourceRequestInfo::ForRequest(request)
           ->GetAssociatedRenderFrame(&render_process_id, &render_frame_id)) {
    NOTREACHED();
    return NULL;
  }

  // We positively need to run the Java creation process in the UI thread.
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&AuthenticationDialogDelegate::ShowDialog, delegate,
                 render_process_id, render_frame_id));

  return delegate;
}

bool OperaResourceDispatcherHostDelegate::HandleExternalProtocol(
    const GURL& url,
    int child_id,
    const content::ResourceRequestInfo::WebContentsGetter& web_contents_getter,
    bool is_main_frame,
    ui::PageTransition page_transition,
    bool has_user_gesture,
    content::ResourceContext* resource_context) {
  return HandleExternalProtocol(url.spec());
}

}  // namespace opera
