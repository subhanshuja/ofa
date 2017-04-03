// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

%{
#include "chill/browser/authentication_dialog.h"
%}

namespace opera {

%feature("director") AuthenticationDialogDelegate;

class AuthenticationDialogDelegate {
 public:
  AuthenticationDialogDelegate(net::URLRequest* request, jobject dialog);
  virtual ~AuthenticationDialogDelegate();

  virtual void OnShow() = 0;
  virtual void OnCancelled() = 0;
  virtual void SetOwner(opera::ChromiumTab* tab) = 0;
};

class AuthenticationDialog {
 public:
  void Accept(const base::string16& username, const base::string16& password);
  void Cancel();

  void SetDelegate(AuthenticationDialogDelegate* delegate);
};

}  // namespace opera
