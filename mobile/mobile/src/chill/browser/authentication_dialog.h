// -*- Mode: c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
// Copyright (C) 2012-2013 Opera Software ASA.  All rights reserved.
//
// This file is an original work developed by Opera Software ASA

#ifndef CHILL_BROWSER_AUTHENTICATION_DIALOG_H_
#define CHILL_BROWSER_AUTHENTICATION_DIALOG_H_

#include <jni.h>

#include "base/android/scoped_java_ref.h"
#include "base/compiler_specific.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string16.h"
#include "content/public/browser/resource_dispatcher_host_login_delegate.h"

namespace net {
class URLRequest;
}  // namespace net

namespace opera {

class ChromiumTab;

class AuthenticationDialogDelegate
    : public content::ResourceDispatcherHostLoginDelegate {
 public:
  AuthenticationDialogDelegate(net::URLRequest* request, jobject dialog);

  // Implement ResourceDispatcherHostLoginDelegate
  void OnRequestCancelled() override;

  // Opera extensions
  virtual void OnShow();
  virtual void OnCancelled();

  // Set which ChromiumTab this delegate belongs to (optional)
  virtual void SetOwner(ChromiumTab* tab);

  void ShowDialog(int render_process_id, int render_view_id);
  void SendAuthToRequester(bool success, const base::string16& username,
                           const base::string16& password);

 private:
  void ShowDialogInternal(ChromiumTab* tab);

  // The request that wants login data.
  // Threading: IO thread.
  net::URLRequest* request_;

  // Reference to the dialog object (to prevent Java GC)
  base::android::ScopedJavaGlobalRef<jobject> j_dialog_;
};

class AuthenticationDialog {
 public:
  void Accept(const base::string16& username, const base::string16& password);
  void Cancel();

  void SetDelegate(AuthenticationDialogDelegate* delegate);

 private:
  scoped_refptr<AuthenticationDialogDelegate> delegate_;
};

}  // namespace opera

#endif  // CHILL_BROWSER_AUTHENTICATION_DIALOG_H_
