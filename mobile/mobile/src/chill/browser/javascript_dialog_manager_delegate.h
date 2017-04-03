// Copyright (c) 2012-2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHILL_BROWSER_JAVASCRIPT_DIALOG_MANAGER_DELEGATE_H_
#define CHILL_BROWSER_JAVASCRIPT_DIALOG_MANAGER_DELEGATE_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/callback_forward.h"
#include "base/strings/string16.h"
#include "content/public/browser/javascript_dialog_manager.h"

namespace opera {

class OperaDialogClosedCallback {
 public:
  void Run(bool success, const base::string16& user_input);

  void SetCallback(
      const content::JavaScriptDialogManager::DialogClosedCallback& callback);

 private:
  content::JavaScriptDialogManager::DialogClosedCallback callback_;
};

class JavaScriptDialogManagerDelegate {
 public:
  JavaScriptDialogManagerDelegate() {}
  virtual ~JavaScriptDialogManagerDelegate() {}

  virtual void CancelActiveAndPendingDialogs() = 0;

  virtual void RunAlertDialog(
      const std::string& origin_url,
      const base::string16& message_text,
      OperaDialogClosedCallback* callback) = 0;

  virtual void RunConfirmDialog(
      const std::string& origin_url,
      const base::string16& message_text,
      OperaDialogClosedCallback* callback) = 0;

  virtual void RunBeforeUnloadDialog(
      bool is_reload,
      OperaDialogClosedCallback* callback) = 0;

  virtual void RunPromptDialog(
      const std::string& origin_url,
      const base::string16& message_text,
      const base::string16& default_prompt_text,
      OperaDialogClosedCallback* callback) = 0;

  virtual void HandleJavaScriptDialog(bool accept,
                                      const base::string16& prompt) = 0;

  virtual bool GetSuppressMessages() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(JavaScriptDialogManagerDelegate);
};

}  // namespace opera

#endif  // CHILL_BROWSER_JAVASCRIPT_DIALOG_MANAGER_DELEGATE_H_
