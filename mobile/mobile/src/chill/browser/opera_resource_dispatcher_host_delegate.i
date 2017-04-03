// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2015 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

// Suppress "Returning a pointer or reference in a director method is not
// recommended."
%warnfilter(473) OperaResourceDispatcherHostDelegate;

%{
#include "chill/browser/opera_resource_dispatcher_host_delegate.h"
%}

SWIG_SELFREF_NAMESPACED_CONSTRUCTOR(opera, OperaResourceDispatcherHostDelegate);

namespace opera {

%feature("director") OperaResourceDispatcherHostDelegate;

class OperaResourceDispatcherHostDelegate {
 public:
  virtual ~OperaResourceDispatcherHostDelegate();

  virtual AuthenticationDialogDelegate* CreateAuthenticationDialog(
      const std::string& challenger, const std::string& realm,
      net::URLRequest* request) = 0;
  virtual bool HandleExternalProtocol(const std::string& url) = 0;
};

}  // namespace opera
