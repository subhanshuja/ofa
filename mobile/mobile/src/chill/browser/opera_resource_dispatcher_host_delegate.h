// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_OPERA_RESOURCE_DISPATCHER_HOST_DELEGATE_H_
#define CHILL_BROWSER_OPERA_RESOURCE_DISPATCHER_HOST_DELEGATE_H_

#include <string>

#include "base/android/scoped_java_ref.h"
#include "base/compiler_specific.h"
#include "content/public/browser/resource_dispatcher_host_delegate.h"

namespace content {
class ResourceContext;
class ResourceThrottle;
}

namespace opera {

class AuthenticationDialogDelegate;

class OperaResourceDispatcherHostDelegate
    : public content::ResourceDispatcherHostDelegate,
      public base::android::ScopedJavaGlobalRef<jobject> {
 public:
  OperaResourceDispatcherHostDelegate() {}

  // ResourceDispatcherHostDelegate implementation.
  void RequestBeginning(
      net::URLRequest* request,
      content::ResourceContext* resource_context,
      content::AppCacheService* appcache_service,
      content::ResourceType resource_type,
      ScopedVector<content::ResourceThrottle>* throttles) override;

  content::ResourceDispatcherHostLoginDelegate* CreateLoginDelegate(
      net::AuthChallengeInfo* auth_info,
      net::URLRequest* request) override;

  bool HandleExternalProtocol(
      const GURL& url,
      int child_id,
      const content::ResourceRequestInfo::WebContentsGetter& web_contents_getter,
      bool is_main_frame,
      ui::PageTransition page_transition,
      bool has_user_gesture,
      content::ResourceContext* resource_context) override;

  // Opera extensions
  virtual AuthenticationDialogDelegate* CreateAuthenticationDialog(
      const std::string& challenger,
      const std::string& realm,
      net::URLRequest* request) = 0;
  virtual bool HandleExternalProtocol(const std::string& url) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(OperaResourceDispatcherHostDelegate);
};

}  // namespace opera

#endif  // CHILL_BROWSER_OPERA_RESOURCE_DISPATCHER_HOST_DELEGATE_H_
