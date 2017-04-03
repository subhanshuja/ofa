// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHILL_BROWSER_OPERA_JAVASCRIPT_DIALOG_MANAGER_H_
#define CHILL_BROWSER_OPERA_JAVASCRIPT_DIALOG_MANAGER_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/callback_forward.h"
#include "base/strings/string16.h"
#include "content/public/browser/javascript_dialog_manager.h"

#include "chill/browser/javascript_dialog_manager_delegate.h"

namespace opera {

class OperaJavaScriptDialogManager : public content::JavaScriptDialogManager {
 public:
  explicit OperaJavaScriptDialogManager(JavaScriptDialogManagerDelegate*);
  virtual ~OperaJavaScriptDialogManager();

  // Inherited from JavaScriptDialogManager
  virtual void RunJavaScriptDialog(
      content::WebContents* web_contents,
      const GURL& origin_url,
      content::JavaScriptMessageType javascript_message_type,
      const base::string16& message_text,
      const base::string16& default_prompt_text,
      const content::JavaScriptDialogManager::DialogClosedCallback& callback,
      bool* did_suppress_message) override;

  virtual void RunBeforeUnloadDialog(
      content::WebContents* web_contents,
      bool is_reload,
      const content::JavaScriptDialogManager::DialogClosedCallback& callback)
      override;

  virtual void CancelActiveAndPendingDialogs(
      content::WebContents* web_contents) override;

  virtual void ResetDialogState(
      content::WebContents* web_contents) override;

  virtual bool HandleJavaScriptDialog(content::WebContents* web_contents,
                                      bool accept,
                                      const base::string16* prompt_override)
                                          override;

  bool GetSuppressMessages() const {
    return delegate_->GetSuppressMessages();
  }

 private:
  OperaDialogClosedCallback callback_;
  JavaScriptDialogManagerDelegate* delegate_;  // unfortunately owned by Java

  // The stub is installed when the WebContents is destroyed
  // to avoid any inadvertent calls to a deleted object.
  static void StubCallback(bool, const base::string16&);

  DISALLOW_COPY_AND_ASSIGN(OperaJavaScriptDialogManager);
};

}  // namespace opera

#endif  // CHILL_BROWSER_OPERA_JAVASCRIPT_DIALOG_MANAGER_H_
